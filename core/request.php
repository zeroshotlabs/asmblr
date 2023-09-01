<?php declare(strict_types=1);
/**
 * @file Request.php A HTTP or CLI request.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;
use asm\types\hostname,asm\types\encoded_str,asm\types\path,asm\types\url;


/**
 * Current request data.
 *
 * The Request DAO encapsulates and normalizes the raw request data for both HTTP
 * and CLI requests.
 *
 * It is used by asmd to calculate application URLs and determine whether the request
 * came from a web browser or from the command line, and is used throughout an app.
 *
 * Request data, like $_GET and $_POST, and headers, are not included.
 *
 * This Struct is a "singleton" - once it's been initialized, it's values are persisted
 * in the $Request static variable (though it can be re-generated).
 */
class request
{
    public readonly bool $IsCLI;

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


    /**
     * @todo PHP needs one-way readonly properties.
     */
    public readonly bool $IsBaseScheme;
    public readonly bool $IsBaseHost;
    public readonly bool $IsBasePath;

    public readonly bool $IsForwarded;
    public readonly bool $IsHTTPS;

    public readonly string $remote_ip;

    public readonly string $endpoint_name;
    public readonly array $argv;
    public readonly int $argc;

    const MIN_ARGC = 3;


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
     * @todo consider SERVER_NAME/port/etc for GAE and similar environments (ports).
     * @todo Handle CLI arguments better, including supporting options like --something, which
     *       will be incorporated into the Config.
     * 
     * @note HTTP auth isn't handled.
     * @note The path is lowercased.
     */
    public function __construct( string $base_url = null )
    {
        if( !empty($_SERVER['argv']) )
        {
            if( $_SERVER['argc'] < self::MIN_ARGC )
                throw new E\Exception("CLI execution requires at least two arguments: php DOC_ROOT/index.php {hostname} {pagename} args ...");

            $this->IsCLI = true;

            // @todo needs testing - also base_url usage?
            $this->url = URL::str($_SERVER['argv'][1]);
            $this->endpoint_name = $_SERVER['argv'][2];

            // @todo need to handle argv better, or similar to how a _GET is done
            // @todo deal with POST?
            $this->argv = $_SERVER['argv'];
            $this->argc= $_SERVER['argc'];
        }
        else
        {
            $this->IsCLI = false;

            $this->IsForwarded = !empty($_SERVER['HTTP_X_FORWARDED_FOR']);

            if( $this->IsForwarded )
            {
                [$this->IsHTTPS,$Scheme] = ($_SERVER['HTTP_X_FORWARDED_PROTO'] === 'https' ? [true,'https'] : [false,'http']);
                $this->remote_ip = $_SERVER['HTTP_X_FORWARDED_FOR'];

                $Host = $_SERVER['HTTP_X_FORWARDED_HOST'] ?? $_SERVER['HTTP_HOST'];
                $Port = $_SERVER['HTTP_X_FORWARDED_PORT'];
            }
            else
            {
                [$this->IsHTTPS,$Scheme] = ($_SERVER['HTTPS'] ?? false === 'on' ? [true,'https'] : [false,'http']);
                $this->remote_ip = $_SERVER['REMOTE_ADDR'];

                $Host = $_SERVER['HTTP_HOST'];
                $Port = $_SERVER['SERVER_PORT'];
            }

            $Path = $_SERVER['DOCUMENT_URI'] ?? 'unknown';

            // @todo does anyone use username:password@ anymore?  :)

            // maintained as the original request, not lower cased
            $this->original_url = new url($Scheme,'','',hostname::str($Host),$Port,path::url($Path),encoded_str::arr($_GET),'');

            // lowercased, canonized by base_url; active URL for processing the request
            $this->url = clone $this->original_url;
            $this->url->path->lower();

            // merge in the baseurl if supplied - this will change $this->url
            if( $base_url )
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
     *  - $route_path - the canonicalized request path, with $base_url path removed and no trailing slash; used for routing the request
     * 
     * The following flags are also set, based on the request and base_url:
     *  - $IsBaseScheme - TRUE if the specified scheme in the base URL matches the request scheme.
     *  - $IsBaseHost - TRUE if the specified hostname in the base URL matches the request host.
     *  - $IsBasePath - TRUE if the specified path in the base URL prefixes the request path.
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
        $this->root_url->path->IsDir = true;
                                                
        if( $this->root_url->scheme !== $this->url->scheme )
        {
            $this->url->scheme = $this->root_url->scheme;
            $this->IsBaseScheme = false;
        }
        else
            $this->IsBaseScheme = true;

        if( ((string) $this->root_url->hostname) !== ((string) $this->url->hostname) )
        {
            $this->url->hostname = $this->root_url->hostname;
            $this->IsBaseHost = false;
        }
        else
            $this->IsBaseHost = true;

        if( strpos((string) $this->url->path,(string) $this->root_url->path) === 0 )
        {
            $this->route_path = clone $this->url->path;
            $this->route_path->IsDir = false;

            if( count($this->root_url->path) )
                $this->route_path->mask($this->root_url->path);
         
            $this->IsBasePath = true;
        }
        else
        {
            // @note The redirect/404/etc is left to the app.  Handle
            // this by checking $IsBasePath.
            $this->route_path = clone $this->url->path;
            $this->route_path->IsDir = false;

            $this->IsBasePath = false;
        }
    }


    /**
     * Determine whether the request appears to be from a mobile device.
     *
     * @retval boolean TRUE if the request appears to be from a mobile device.
     * @retval NULL HTTP_USER_AGENT wasn't set in $_SERVER.
     *
     * @todo Review and optimize (possibly getting rid of the regex).
     */
    public static function IsMobile(): bool
    {
        if( empty($_SERVER['HTTP_USER_AGENT']) )
            return false;

        foreach( ['iPhone','iPad','iPod','Android'] as $ua )
            if( stripos($_SERVER['HTTP_USER_AGENT'],$ua) > 0 )
                return true;

        return false;
    }

    /**
     * Determine if the request is ChromePHP (chromelogger.com) aware.
     *
     * @note ChromePHP doesn't currently announce itself so this has little use and isn't supported anymore.
     *       https://github.com/ccampbell/chromelogger/issues/8
     * @note Manually toggle LogPublic config variable to TRUE/FALSE until they fix this.
     * @todo Revise.
     */
    public static function IsChromePHP()
    {
        return TRUE;

        if( !empty($_SERVER['HTTP_USER_AGENT']) )
            return (bool) preg_match('{\bChrome/\d+[\.\d+]*\b}',$_SERVER['HTTP_USER_AGENT']);
    }

    /**
     * Determine if the request is FirePHP aware.
     *
     * @retval bool TRUE if the request is FirePHP aware.
     * @todo revise
     */
    public static function IsFirePHP()
    {
        if( isset($_SERVER['HTTP_X_FIREPHP_VERSION']) || (!empty($_SERVER['HTTP_USER_AGENT']) && strpos($_SERVER['HTTP_USER_AGENT'],'FirePHP') !== FALSE) )
            return TRUE;
    }
}


/**
 * Normalize and manage file upload information.
 *
 * The FileUpload Struct encapsulates information about one or
 * multiple file uploads as found in PHP's $_FILES superglobal.
 * 
 * @todo revise or delete.
 */
abstract class FileUpload
{
    protected static $Skel = array('Filename'=>'','ContentType'=>'','TmpPath'=>'',
                                   'Error'=>0,'FileSize'=>0);

    /**
     * @var array $Err2Txt
     * Error code constants to text mappings.
     */
    protected static $Err2Txt = array(UPLOAD_ERR_OK=>'Ok',UPLOAD_ERR_INI_SIZE=>'Upload too big (UPLOAD_ERR_INI_SIZE)',
                                      UPLOAD_ERR_FORM_SIZE=>'Upload too big (UPLOAD_ERR_FORM_SIZE)',
                                      UPLOAD_ERR_PARTIAL=>'Partial upload (UPLOAD_ERR_PARTIAL)',
                                      UPLOAD_ERR_NO_FILE=>'No file upload (UPLOAD_ERR_NO_FILE)',
                                      UPLOAD_ERR_NO_TMP_DIR=>'Missing temp. directory (UPLOAD_ERR_NO_TMP_DIR)',
                                      UPLOAD_ERR_CANT_WRITE=>'Failed to write to disk (UPLOAD_ERR_CANT_WRITE)',
                                      UPLOAD_ERR_EXTENSION=>'A PHP extension stopped the upload (UPLOAD_ERR_EXTENSION)');

    /**
     * Get data about the current request's uploaded files.
     *
     * The $_FILES superglobal is used automatically.  For multiple file
     * uploads, the name of the form field is assumed to be of the
     * FieldName[] naming convention.
     *
     * @param string $Name The name of the file upload form field or empty to take all of $_FILES.
     * @retval array An array of one or more FileUpload Structs, or an empty array if no files have been uploaded.
     */
    public static function Init( $Name )
    {
        if( empty($_FILES) )
            return array();

        $Files = array();

        // multiple files, each with a different name (none [] syntax)
        if( empty($Name) )
        {
            foreach( $_FILES as $K => $V )
            {
                if( $V['error'] === UPLOAD_ERR_NO_FILE )
                    continue;

                $F = static::$Skel;
                $F['Filename'] = $V['name'];
                $F['ContentType'] = $V['type'];
                $F['TmpPath'] = $V['tmp_name'];
                $F['Error'] = $V['error'];
                $F['FileSize'] = $V['size'];
                $Files[$K] = $F;
            }
        }
        // Multiple file uploads
        else if( is_array($_FILES[$Name]['name']) )
        {
            foreach( $_FILES[$Name]['name'] as $K => $V )
            {
                if( $_FILES[$Name]['error'][$K] === UPLOAD_ERR_NO_FILE )
                    continue;

                $F = static::$Skel;
                $F['Filename'] = $_FILES[$Name]['name'][$K];
                $F['ContentType'] = $_FILES[$Name]['type'][$K];
                $F['TmpPath'] = $_FILES[$Name]['tmp_name'][$K];
                $F['Error'] = $_FILES[$Name]['error'][$K];
                $F['FileSize'] = $_FILES[$Name]['size'][$K];
                $Files[] = $F;
            }
        }
        // Single file upload
        else
        {
            if( $_FILES[$Name]['error'] === UPLOAD_ERR_NO_FILE )
                return array();

            $F = static::$Skel;
            $F['Filename'] = $_FILES[$Name]['name'];
            $F['ContentType'] = $_FILES[$Name]['type'];
            $F['TmpPath'] = $_FILES[$Name]['tmp_name'];
            $F['Error'] = $_FILES[$Name]['error'];
            $F['FileSize'] = $_FILES[$Name]['size'];
            $Files[] = $F;
        }

        return $Files;
    }

    /**
     * Check if a particular FileUpload Struct is free from upload errors.
     *
     * @param array $File FileUpload Struct to check.
     * @retval boolean TRUE if the file upload was successful.
     */
    public static function IsOK( $File )
    {
        return (isset($File['Error']) && $File['Error'] === UPLOAD_ERR_OK)?TRUE:FALSE;
    }

    /**
     * Return an error message for one of PHP's file upload error constants.
     *
     * @param int $Err The PHP error constant.
     * @retval string The error message or Unknown.
     */
    public static function Err2Txt( $Err )
    {
        if( isset(static::$Err2Txt[$Err]) )
            return static::$Err2Txt[$Err];
        else
            return 'Unknown';
    }
}

