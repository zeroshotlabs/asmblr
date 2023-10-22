<?php declare(strict_types=1);
/**
 * @file request.php A HTTP or CLI request.
 * @author @zaunere Zero Shot Labs
 * @version 5.0
 * @copyright Copyright (c) 2023 Zero Shot Laboratories, Inc. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;
use asm\_e\e400,asm\types\hostname,asm\types\encoded_str,asm\types\path,asm\types\url;
use asm\http\http_headers;
use asm\config;

/**
 * Current request data.
 *
 * The request class encapsulates and normalizes the raw request data for both HTTP
 * and CLI requests.
 *
 * It is used by app to calculate application URLs and determine whether the request
 * came from a web browser or from the command line, and is used throughout an app.
 *
 * Request data, like  $_POST and headers, are not included.
 */
class request
{
    use http_headers;

    public bool $is_cli;

    // the original request, untouched
    public url $original_url;

    // canonized URL according to base_url - used for redirect
    public url $url;

    // canonized root URL according to request, used for linking
    public url $root_url;

    // base URL template from config.
    public string $base_url;

    // path used for matching an endpoint
    public path $route_path;

    // true if the request's scheme matches base_url
    public bool $is_base_scheme;

    // true if the request's host matches base_url
    public bool $is_base_host;

    // true if the request's base path matches base_url
    public bool $is_base_path;

    // true if the request was forwarded by a proxy
    public bool $is_forwarded;

    // true if the end user request was over HTTPS
    public bool $is_https;

    // the end user's IP
    public string $remote_ip;

    // the name of the CLI endpoint to execute
    public string $endpoint_name;

    public array $argv;
    public int $argc;
    const MIN_ARGC = 2;


    /**
     * Build up and normalize data about the current request.
     * 
     * A request can either be HTTP or CLI.
     *
     * When executed from the CLI, the first argument (argv[1]) must be the hostname
     * of the app, for example hostname.com, and the second argument (argv[2]) must be
     * the page name.  Any additional arguments are passed to the page as a query string.
     * 
     * @property $original_url The original request URL.
     * @property $url The active request URL, canonicalized by $base_url; used to determine redirects.
     * 
     * @note HTTP auth isn't handled.
     * @note The request/route path is lowercased and potentially modified by base_url.
     * @todo consider SERVER_NAME/port/etc for GAE and similar environments (ports).
     */
    public function __construct( string $base_url = '' )
    {
        if( self::is_cli() )
        {
            if( $_SERVER['argc'] < self::MIN_ARGC )
                throw new e400("asmblr requires at least one argument:\n   php {$_SERVER['argv'][0]} {endpoint} args ...");

            $this->is_cli = true;
            $this->endpoint_name = $_SERVER['argv'][1];

            // @todo need to handle argv better, or similar to how a _GET is done
            $this->argv = $_SERVER['argv'];
            $this->argc= $_SERVER['argc'];

            // build out empty/generic placeholders
            $this->url = $this->original_url = url::str('cli://localhost');
            $this->use_base_url('localhost');
        }
        else
        {
            $this->is_cli = false;

            $this->is_forwarded = !empty($_SERVER['HTTP_X_FORWARDED_FOR']);

            if( $this->is_forwarded )
            {
                [$this->is_https,$scheme] = ($_SERVER['HTTP_X_FORWARDED_PROTO'] ?? '' === 'https' ? [true,'https'] : [false,'http']);
                $this->remote_ip = $_SERVER['HTTP_X_FORWARDED_FOR'];

                $host = $_SERVER['HTTP_X_FORWARDED_HOST'] ?? $_SERVER['HTTP_HOST'];
                $port = $_SERVER['HTTP_X_FORWARDED_PORT'] ?? $_SERVER['SERVER_PORT'];
            }
            else
            {
                [$this->is_https,$scheme] = ($_SERVER['HTTPS'] ?? false === 'on' ? [true,'https'] : [false,'http']);
                $this->remote_ip = $_SERVER['REMOTE_ADDR'];

                $host = $_SERVER['HTTP_HOST'];
                $port = $_SERVER['SERVER_PORT'];
            }

            $path = $_SERVER['DOCUMENT_URI'] ?? '/?-?';

            // @todo does anyone use username:password@ anymore?  one day it can be added around here somewhere

            // maintained as the original request, not altered; keep case for FES requests
            // @note $_POST and other methods arent touched, left to the app
            $this->original_url = new url($scheme,'','',hostname::str($host),$port,path::url($path),encoded_str::arr($_GET),'');

            // active URL used for routing - lowercased, canonized by base_url;
            $this->url = clone $this->original_url;
            $this->url->path->lower();

            // merge in the baseurl potentially changing $this->url
            $this->use_base_url($base_url);
        }
    }


    /**
     * Canonicalize the request around a configured base URL, including setting flags and creating utility URLs.
     * 
     * This should always be called, even if base_url is wildcard/blank.
     * 
     * The base_url is a template URL that can specify:
     *  - A scheme, either http:// or https://, or an asterisk to use the requested scheme.
     *  - A hostname, or an asterisk to use the requested hostname.
     *  - A path, which is prepended to the path of the request, or a root path.
     * 
     * The base_url is merged with the request's, to form the canonical request URL of the request:
     *   - If the $base_url specifies one of the components above, and the request's
     *     component doesn't match, the request's component is replaced by the $base_url component.
     * 
     * The following utility variables are involved:
     *  - $base_url - configured template for the site's root URL
     *  - $url = canonicalized request URL
     *  - $root_url - canonicalized root URL of the site, always with a trailing '/', used for link creation, redirects
     *  - $route_path - the canonicalized request path, with $base_url path removed and no trailing slash, lowercased; used for routing the request
     * 
     * The following flags are also set, based on the request and base_url:
     *  - $is_base_scheme - TRUE if the specified scheme in the base URL matches the request scheme.
     *  - $is_base_host - TRUE if the specified hostname in the base URL matches the request host.
     *  - $is_base_path - TRUE if the specified path in the base URL prefixes the request path.
     *
     * @param string $base_url Configured base URL.
     *
     * @property string $base_url The configured base URL template.
     * @property \asm\types\url $root_url The site root, based on the $base_url and current request.
     * @property \asm\types\url $url The request URL, canonicalized by $base_url.
     * 
     * @note Because there's no trailing slash on route_path, endpoints of /admin/ and /admin will both
     *       match for both URLs.  It also means that /admin won't match for /admin/.
     * @note Username/password is not considered and not merged.
     * 
     * @todo test with a blank base_url vs all wildcards.  possibly have some options for dev mode
     */
    public function use_base_url( string $base_url ): void
    {
        $this->root_url = url::str(str_replace(['*:/','/*/'],
                                               [$this->url->scheme.':/','/'.$this->url->hostname.($this->url->port?':'.$this->url->port:'').'/'],
                                                $base_url));
        $this->root_url->path->is_dir = true;
                                                
        if( $this->root_url->scheme !== $this->url->scheme )
        {
            $this->url->scheme = $this->root_url->scheme;
            $this->is_base_scheme = false;
        }
        else
            $this->is_base_scheme = true;

        if( ((string) $this->root_url->hostname) !== ((string) $this->url->hostname) )
        {
            $this->url->hostname = $this->root_url->hostname;
            $this->is_base_host = false;
        }
        else
            $this->is_base_host = true;

        if( strpos((string) $this->url->path,(string) $this->root_url->path) === 0 )
        {
            $this->route_path = clone $this->url->path;
            $this->route_path->is_dir = false;

            if( count($this->root_url->path) )
                $this->route_path->mask($this->root_url->path);
         
            $this->is_base_path = true;
        }
        else
        {
            // @note The redirect/404/etc is left to the app.  Handle
            // this by checking $is_base_path.
            $this->route_path = clone $this->url->path;
            $this->route_path->is_dir = false;

            $this->is_base_path = false;
        }
    }


    /**
     * Determine if a request appears to be for a FES resource.  This is case sensitive.
     * 
     * This will return true if the first request path's segment ISN'T found as an endpoint URL.
     * 
     * Front End Stack (FES) resources are directly output without being PHP processed.
     * 
     * Path rewriting can be performed to determine the resource to return, however be mindful
     * of configured endpoints to avoid collisions.  See \asm\filesystem and reroot.
     * 
     * A root request returns false.
     * 
     * @param config $config The config object.
     * @param request $request The request object.
     * @return bool false if the request's first segment was found in the endpoint <map name="
     * @return int The line number the match was found.  This corresponds to the endpoint's index in $endpoint_map.
     * 
     * @todo Make the length to compare variable (3 segments, etc).
     * @todo Optimize routing because this is executed first.
     * @note This isn't bulletproof especially with large number of resources/endpoints.
     * @note Advanced processing - such as running resources through PHP - can be performed
     *       using a combination of routing endpoints and this.
     */
    public static function is_fes( config $config,request $request ): int|bool
    {
        if( $request->is_cli || (($first_segment = $request->url->path[0]) === '/') )
            return false;

        $p1 = strpos($config->endpoints_url_blob,PHP_EOL.'/'.$first_segment);

        if( $p1 === false )
            return false;
        else 
            return substr_count($config->endpoints_url_blob,PHP_EOL,0,$p1+1);
    }


    /**
     * Determine whether the request appears to be from a mobile device.
     *
     * @retval boolean TRUE if the request appears to be from a mobile device.
     * @retval NULL HTTP_USER_AGENT wasn't set in $_SERVER.
     *
     * @todo Review and optimize (possibly getting rid of the regex).
     */
    public static function is_mobile(): bool
    {
        if( empty($_SERVER['HTTP_USER_AGENT']) )
            return false;

        foreach( ['iPhone','iPad','iPod','Android'] as $ua )
            if( stripos($_SERVER['HTTP_USER_AGENT'],$ua) !== false )
                return true;

        return false;
    }

    /**
     * Determine if the request is from the commabnd line.
     */
    public static function is_cli()
    {
        return !empty($_SERVER['argv']);
    }

    /**
     * Determine if the request is ChromePHP (chromelogger.com) aware.
     *
     * @note ChromePHP doesn't currently announce itself so this has little use and isn't supported anymore.
     *       https://github.com/ccampbell/chromelogger/issues/8
     * @note Manually toggle LogPublic config variable to TRUE/FALSE until they fix this.
     * @todo Revise.
     */
    public static function is_chromephp()
    {
        if( !empty($_SERVER['HTTP_USER_AGENT']) )
            return (bool) preg_match('{\bChrome/\d+[\.\d+]*\b}',$_SERVER['HTTP_USER_AGENT']);
    }

    /**
     * Determine if the request is FirePHP aware.
     *
     * @retval bool TRUE if the request is FirePHP aware.
     * @todo revise
     */
    public static function is_firephp()
    {
        if( isset($_SERVER['HTTP_X_FIREPHP_VERSION']) || (!empty($_SERVER['HTTP_USER_AGENT']) && strpos($_SERVER['HTTP_USER_AGENT'],'FirePHP') !== FALSE) )
            return TRUE;
    }
}
