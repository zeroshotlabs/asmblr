<?php
/**
 * @file cnvyrc.inc Client for the cnvyr.io API.
 * @author Stackware, LLC
 * @version 4.2
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * cnvyr client (cnvyrc) provides connectivity and caching for the
 * <a href="http://cnvyr.io">cnvyr.io minify thumbnail API</a> and provides a static
 * Page method for serving assets.
 *
 * The non-static methods are for communicating with the API and getting a result.
 *
 * The static methods are for serving from cache, hard-caching a binary, and a Page function.
 *
 * The following configuration directives must be set in the manifest's Config tab.
 *
 *  - @c cnvyrBaseURL: full URL of cnvyr.io API, typically http://srv.cnvyr.io/v1
 *  - @c cnvyrCacheLocal: boolean TRUE or FALSE whether to cache results from cnvyr.io
 *  - @c cnvyrCacheDir: local filesystem directory to cache assests if above is TRUE.
 *                      Must be writeable by the web processes.
 *  - @c cnvyrHandlerPrefix: A short string to prefix each cached resource.  Must be
 *                           unique per server.
 *  - @c cnvyrOrigins: One or more \c handler=directory/ entries that map a URL's first
 *                     segment to a local directory where the assets reside.
 *  - @c cnvyrOpOverrides: A key/value array of ops/values that will get explicitly set for all API calls.
 *  - @c cnvyrHTTPCacheTime: Number of seconds of @e absolute HTTP caching (see asm::HTTP::Cache).
 *
 * This class must be extended, at least as a stub.
 *
 * Ehe cnvyrc::$XAccelRedirectBase static property should be set in the stub class.
 *
 * All caching is done to a local filesystem path.
 *
 * The cnvyrc::FromCache can be called statically in an application's Load.inc which will
 * skip the rest of asmblr execution.
 *
 * By default asmblr lowercases all requested URLs.  When serving assets on Linux with capitalized
 * letters, the asset won't be found.  The following will recalculate the request, keeping it's
 * original case, and can be added to the cnvyr.io short-circuit in Load.inc:
 *       @code
 *       $Request = \asm\Request::Init();
 *       Request::CalcURLs($app->Request,$app->Config['BaseURL'],FALSE);
 *       @endcode
 *
 * cnvyrc also supports CSS or JS bundles which combine multiple files into one and perform operations
 * on the result (asset pipeline).  Bundles are defined in the stub class using the cnvyrc::$Bundles property
 * with the following structure:
 *  @code
 *  public $Bundles = array('bundle.css'=>
 *                      array(array('css_bootstrap.min','css_jquery-ui-1.10.3.custom','css_style'),array('min'=>'css','gzip'=>'1')),
 *                          'bundle.js'=>
 *                      array(array('js_jquery-1.10.2.min','js_jquery-ui-1.10.3.custom.min','js_bootstrap.min','js_sitelib'),array('min'=>'js','gzip'=>'1')));
 * @endcode
 *
 * A bundle is referenced by it's key, ie. @c bundle.css or @c bundle.js in the example above.  Each filepart
 * of a bundle is referenced but it's template name.
 *
 * @note Pre-gzip'd assets (those gzip'd by the API) and using x-sendfile will only work with nginx.
 * @note If @c cnvyrCacheDir is set to a path in /tmp the following may be useful to avoid automatic deletion of
 *       cached assets.  Add the following flag to the first call of tmpwatch, generally in /etc/cron.daily/tmpwatch
 *       @code
 *       -x /tmp/cnvyr-cache
 *       @endcode
 *
 * @todo Cache-busting URLs aren't handled.
 * @todo $Bundles probably doesn't belong in cnvyrc as a property since it's more dependant on the srv Page function
 *       and uses template names which cnvyrc itself doesn't care about.  Should be defined somewhere else.
 */
abstract class cnvyrc extends restr
{
    /**
     * @var string $XAccelRedirectBase
     * Base URL for serving via nginx's X-Accel-Redirect.  Needs leading and trailing
     * slashes and must match nginx config (see nginx-asmblr.conf).
     *
     * Configurable in the stub class only.
     */
    protected static $XAccelRedirectBase = '/xcnvyr/';

    /**
     * @var array $Config
     * Configuration variables from the manifest.
     */
    protected $Config = array();

    /**
     * @var array $Bundles
     * Definition of CSS or JS bundles.
     *
     * Configurable in the stub class only.
     */
    public $Bundles = array();

    /**
     * Generated from the manifest's cnvyrOrigins config directive.
     * Generally used in the static cnvyrc::srv() Page method.
     *
     * @note CSS/JS entries typically aren't needed since they're rendered through the templating system.
     */
    public $Origins = array();


    /**
     * Instantiate a cnvyrc object for API communication and caching of the result.
     *
     * @param App $app The application's App object.
     */
    public function __construct( $app )
    {
        $this->Config = $app->Config;

        if( !empty($this->Config['cnvyrOrigins']) )
        {
            foreach( $this->Config['cnvyrOrigins'] as $V )
            {
                list($H,$D) = explode('=',$V);
                $this->Origins[trim($H)] = trim($D);
            }
        }

        parent::__construct($this->Config['cnvyrAPIURL']);
    }

    /**
     * Perform an API request with one or more buckets of content.
     *
     * Each bucket of content is a string of data to send to the API.  For CSS/JS,
     * an array of strings is acceptable as a bundle.  For images, only a single bucket
     * is accepted.
     *
     * This will trigger a HTTP 500 error and exit if the API call fails.
     *
     * @param string $Payload A string that's the payload to operate on.
     * @param array $Payload An array of strings to bundle and perform operations on (CSS/JS only).
     * @param array $Ops Associative array of ops for the API to perform.
     * @retval string The processed result from the cnvyr.io API.
     */
    public function ToAPI( $Payload,$Ops )
    {
        // treat as a bundle
        if( is_array($Payload) === TRUE )
        {
            $Bundle = array();
            $i = 0;
            foreach( $Payload as $K => $V )
            {
                $Bundle['@files'.$i] = $V;
                ++$i;
            }

            $Response = $this->POSTFiles('',$Ops,array(),$Bundle);
        }
        // single
        else
        {
            $Response = $this->POSTFiles('',$Ops,array(),array('@files0'=>$Payload));
        }

        if( !empty($this->CURLError) )
        {
            llog($Response);
            HTTP::_500();
        }

        return $Response;
    }

    /**
     * Cache a payload.
     *
     * The payload can be any string, typically a result from the API, or a string that
     * doesn't require any processing (PDF, fonts, doc, etc).  Cached payloads can then
     * be served by FromCache().
     *
     * @param string $Payload The payload to cache.
     * @param string $Filename The cache filename.
     * @retval int The number of bytes written to cache.
     * @retval boolean FALSE if the save failed.
     *
     * @note Existing cache files are silently overwritten.
     */
    public function ToCache( $Payload,$Filename )
    {
        return file_put_contents($this->Config['cnvyrCacheDir'].$Filename,$Payload);
    }

    /**
     * Attempt a local cache hit and serve the cached resource if available.
     *
     * This should typically be called in an application's Load.inc which will skip the rest of
     * asmblr execution for a very fast response.
     *
     * @param string $Filename The filename of the cached resource to be checked and served.
     * @param string $CacheDir The full path to the cache directory for cnvyr with trailing slash.
     * @param boolean $DirectOut TRUE to avoid using any server optimization (i.e. X-SendFile).
     * @retval boolean TRUE if the cache hit was successfully served, FALSE if it didn't exist.
     *
     * @note $Filename and $CacheDir are trusted - do checks elsewhere.
     * @note Serving gzip'd cache files using Apache's mod_xsendfile won't work because it strips Content-Encoding.
     * @todo Content-Encoding: gzip doesn't appear to be set, even when using nginx.
     */
    public static function FromCache( $Filename,$CacheDir,$DirectOut = FALSE )
    {
        if( is_readable($CacheDir.$Filename) === FALSE )
            return FALSE;

        // Is this proper?
        if( !empty($GLOBALS['asmapp']->Config['cnvyrHTTPCacheTime']) )
            HTTP::Cache($GLOBALS['asmapp']->Config['cnvyrHTTPCacheTime']);

        // we've decided to explicitly set the content-type for all SAPIs because it's otherwise
        // too unpredictable
        HTTP::ContentType(HTTP::Filename2ContentType($Filename));

        if( $DirectOut )
        {
            header('Content-Length: '.filesize($CacheDir.$Filename));
            readfile($CacheDir.$Filename);
        }
        // nginx
        else if( PHP_SAPI === 'fpm-fcgi' )
        {
            // nginx generally sets correct etag/last-modified/content-length headers based on the cached file
            header('X-Accel-Redirect: '.static::$XAccelRedirectBase.$Filename);
        }
        // apache2 with mod_xsendfile
        else if( PHP_SAPI === 'apache2handler' )
        {
            // this should automatically set correct content-length, etag, vary, etc.
            header("X-SendFile: {$CacheDir}{$Filename}");
        }
        // everything else - same as when DirectOut is TRUE
        else
        {
            header('Content-Length: '.filesize($CacheDir.$Filename));
            readfile($CacheDir.$Filename);
        }

        return TRUE;
    }

    /**
     * Default Page function for cnvyr.io based asset delivery.
     *
     * This includes logic for pushing local assets to the cnvyr.io API, caching the result
     * if desired, and then serving the result.
     *
     * It uses cnvyrc::$Origins to determine where the asset resides on the local filesystem.  The
     * first segment of the requested URL (Request $MatchPath) is used to determine how to handle the asset.
     * The following handlers exist by default:
     *  - img: handle as an image (pulled from filesystem)
     *  - css: handle as CSS (text string from templating system)
     *  - js: handle as Javascript (text string from templating system)
     *
     * Any other prefix is handled as a generic binary and simply cached - no operations are performed.
     *
     * For different handling of the path, you'll need to override this method.
     *
     * This enforces the cnvyrOpOverrides config parameter and sets the cnvyrHTTPCacheTime
	 * parameter from the manifest.
     *
     * @param asm::App $app The application's App object.
     * @param array $OverrideOps Key/value set of ops to set explicitly as full ops, overriding even cnvyrOpOverrides
     *              config setting.  Used when called from a child method.
     * @param array $Path Array of path segments to override the request.  Used whenc alled from a child method.
     *
     * @note In apps that use cnvyrc::FromCache() in Load.inc and a cache entry exists, execution never
     * reaches this method - the serve happens directly in FromCache().
     * @note This exits when complete.
     * @todo This needs to be easier to extend/override.
     */
    public static function srv( \asm\App $app,$OverrideOps = array(),$Path = array() )
    {
        header_remove();

        if( !empty($app->Config['cnvyrHTTPCacheTime']) )
            HTTP::Cache($app->Config['cnvyrHTTPCacheTime']);

        if( empty($Path) )
            $Path = $app->Request['MatchPath']['Segments'];

        // wouldn't make any sense
        if( empty($Path[1]) )
        {
            HTTP::_404();
            exit;
        }

        // instantiate ourselves and perform api/cache/serve
        // global namespace is required so that we hit our stub class
        $cc = new \cnvyrc($app);

        // CSS/JS are handled via the template system, not directly from disk
        if( $Path[0] === 'css' || $Path[0] === 'js' )
        {
            // we might have a bundle - read each buckets from disk
            if( isset($cc->Bundles[$Path[1]]) )
            {
                $Payload = array();
                foreach( $cc->Bundles[$Path[1]][0] as $K => $F )
                {
                    if( isset($app->html->$F) )
                    {
                        $Payload[$Path[1].$K] = $app->html->Render($F);
                    }
                    else
                    {
                        llog("Template {$F} of bundle {$Path[1]} not found");
                        HTTP::_404();
                        exit;
                    }
                }

                // Ops from bundle definition
                $Ops = empty($cc->Bundles[$Path[1]][1])?array():$cc->Bundles[$Path[1]][1];
            }
            else
            {
                $Token = "{$Path[0]}_".str_replace(".{$Path[0]}",'',$Path[1]);

                if( isset($app->html->$Token) )
                {
                    $Payload = $app->html->Render($Token);
                }
                else
                {
                    llog("Template {$Token} not found");
                    HTTP::_404();
                    exit;
                }

                // hardwired default - could be a config somewhere
                $Ops = array('min'=>$Path[0],'gzip'=>'1');
            }
        }
        // handle an image or other binary data
        else
        {
            if( empty($cc->Origins[$Path[0]]) )
            {
                HTTP::_404();
                exit;
            }

            $Origin = "{$app->AppRoot}/{$cc->Origins[$Path[0]]}";

            // TODO: security issue?  I think Request::MatchPath is well cleaned.
            if( is_readable("{$Origin}{$Path[1]}") )
            {
                $Payload = file_get_contents("{$Origin}{$Path[1]}");
            }
            else
            {
                HTTP::_404();
                exit;
            }

            if( $Path[0] === 'img' )
            {
                // hardwired default - could be a config somewhere
                $Ops = array('opt'=>'1');
            }
            else
                $Ops = array();
        }

        // build up cache'd filename - sort of hardwired
        $CacheFile = "{$app->Config['cnvyrPrefix']}_{$Path[0]}_{$Path[1]}";

        if( !empty($OverrideOps) )
            $Ops = $OverrideOps;

        // now either hit the cnvyr API or just locally cache if there are no ops
        if( empty($Ops) )
        {
            if( !empty($app->Config['cnvyrCacheLocal']) )
            {
                $cc->ToCache($Payload,$CacheFile);
            }
        }
        else
        {
            if( !empty( $app->Config['cnvyrOpOverrides']) )
            {
                $app->Config['cnvyrOpOverrides'] = (array) $app->Config['cnvyrOpOverrides'];

                // apply op overrides
                if( !empty($app->Config['cnvyrOpOverrides']) && empty($OverrideOps))
                {
                    foreach( $app->Config['cnvyrOpOverrides'] as $K => $V )
                    {
                        list($L,$L2) = explode('=',$V);
                        $Ops[trim($L)] = trim($L2);
                    }
                }
            }

            // make the call
            $Payload = $cc->ToAPI($Payload,$Ops);

            if( !empty($app->Config['cnvyrCacheLocal']) )
            {
                $cc->ToCache($Payload,$CacheFile);
            }
        }

        // now just output for this request and we're done
        // this type of stuff is done in FromCache() or by the web server itself sometimes
        header('Content-Length: '.strlen($Payload));
        HTTP::ContentType(HTTP::Filename2ContentType($Path[1]));

        if( !empty($Ops['gzip']) )
            header('Content-Encoding: gzip');

        echo $Payload;
        exit;
    }

    /**
     * Parse a path into cnvyr components.
     *
     * This requires a Path Struct, typically the request's MatchPath, and expects a path in the form:
     *   /handler-url/{optional-cache-buster}/filename.ext | bundle-token
     *
     * A cache-buster is generally otherwise ignored in serving the asset.
     *
     * The resource filename/bundle-token is rawurldecode()'d and stripped of '..' and '/' characters.
     *
     * @param array $P Path Struct to parse.
     * @retval array Numeric array with handler, filename/bundle and cache buster elements.
     * @retval NULL The path could not be parsed.
     *
     * @deprecated This isn't used currently but may be reinstated for cache-busting and security purposes.
     */
    public static function PathParse( $P )
    {
        // /handler-url/cache-buster pattern
        if( !empty($P['Segments'][2]) )
        {
            $CacheBuster = $P['Segments'][1];
            $Handler = str_replace(array('..','/'),'',rawurldecode($P['Segments'][0]));
            $Filename = str_replace(array('..','/'),'',rawurldecode($P['Segments'][2]));
        }
        // /handler/filename pattern
        else if( !empty($P['Segments'][1]) )
        {
            $Filename = str_replace(array('..','/'),'',rawurldecode($P['Segments'][1]));
            $Handler = str_replace(array('..','/'),'',rawurldecode($P['Segments'][0]));
            $CacheBuster = '';
        }
        // nothing (probably a 404)
        else
        {
            return array();
        }

        return array($Handler,$Filename,$CacheBuster);
    }
}


/**
 * Create URLs for cnvyrc managed assets.
 *
 * @note Largely similar to LinkPage.  Modifications to __invoke().
 */
class Linkcnvyr extends LinkPage
{
    /**
     * Linkcnvyr constructor.
     *
     * @param PageSet $PageSet PageSet containing Pages to create URLs for.
     * @param App $App The application's App object.
     * @param array $BaseURL URL Struct to use as BaseURL.
     * @param string $BaseURL URL string to use as BaseURL.
     * @param NULL $BaseURL App::$SiteURL will be used.
     * @param string $Buster Numeric string to use as a cache-buster path segment (first segment).
     * @param NULL $Buster No cache-buster will be used.
     *
     * @todo Buster stuff needs work, including properly appending the path segment and perhaps a review
     *       of URL::Set() and Path::Set() stuff.
     */
    public function __construct( \asm\PageSet $PageSet,\asm\App $App,$BaseURL = NULL,$Buster = NULL )
    {
        $this->PageSet = $PageSet;
        $this->App = $App;

        if( !empty($Buster) )
            $this->SetBaseURL($BaseURL,">$Buster");
        else
            $this->SetBaseURL($BaseURL);
    }

    /**
     * cnvyr specific link creation.
     *
     * Bundle example: <?=$lc('css','css-all')?>
     *   File example: <?=$lc('css','style.css')?>
     *
     * @param string $Handler The name of the handler page, typically one of css, img, js or bin.
     * @param string $Filename The filename of the resource or bundle to serve.
     * @retval string The absolute URL of the cnvyr served resource.
     */
    public function __invoke( $Handler = NULL,$Filename = '' )
    {
        $Base = $this->BaseURL;

        if( empty($Handler) || empty($this->PageSet->Pages[$Handler]) )
        {
            Path::Append("PAGE-{$Handler}-NOT-FOUND",$Base['Path']);
            return URL::ToString($Base);
        }

        Path::Merge($this->PageSet->Pages[$Handler]['PathStruct'],$Base['Path']);

//        $Base['Path']['Segments'][] = $Handler;
        $Base['Path']['Segments'][] = $Filename;
        $Base['Path']['IsDir'] = $Base['Path']['IsAbs'] = FALSE;

        return URL::ToString($Base);
    }
}

