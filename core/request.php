<?php
/**
 * @file Request.php A HTTP or CLI request.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;
use asm\types\hostname,asm\types\encoded,asm\types\path;




/**
 * Current request data.
 *
 * The Request DAO encapsulates and normalizes the raw request data for both HTTP
 * and CLI requests.
 *
 * It is used by asmd to calculate application URLs and determine whether the request
 * came from a web browser or from the command line, and is used throughout an app.
 *
 * Request data, like $_GET and $_POST, nor headers, are included.
 *
 * This Struct is a "singleton" - once it's been initialized, it's values are persisted
 * in the $Request static variable (though it can be re-generated).
 */
class Request
{
    public readonly \asm\URL $URL;

//    'SiteURL'=>array(),'BaseURL'=>array(),'MatchPath'=>array(),
//    'IsBaseScheme'=>FALSE,'IsBaseHostname'=>FALSE,'IsBasePort'=>FALSE,'IsBasePath'=>FALSE);

    public readonly bool $IsCLI;
    public readonly string $PageName;
    public readonly array $argv;
    public readonly int $argc;

    public readonly bool $IsForwarded;
    public readonly bool $IsHTTPS;
    public readonly string $RemoteIP;

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
     * @todo consider SERVER_NAME/port/etc for GAE and similar environments (ports).
     * @todo Handle CLI arguments better, including supporting options like --something, which
     *       will be incorporated into the Config.
     * @note HTTP auth isn't handled.
     */
    public function __construct()
    {
        if( !empty($_SERVER['argv']) )
        {
            if( $_SERVER['argc'] < self::MIN_ARGC )
                throw new Exception("CLI execution requires at least two arguments: php DOC_ROOT/index.php {hostname} {pagename} args ...");

            $this->IsCLI = true;

            $this->URL = URL::str($_SERVER['argv'][1]);
            $this->PageName = $_SERVER['argv'][2];

            // @todo need to handle argv better, or similar to a _GET
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
                $this->RemoteIP = $_SERVER['HTTP_X_FORWARDED_FOR'];

                $Host = $_SERVER['HTTP_X_FORWARDED_HOST'];
                $Port = (int) $_SERVER['HTTP_X_FORWARDED_PORT'];
            }
            else
            {
                [$this->IsHTTPS,$Scheme] = ($_SERVER['HTTPS'] ?? false === 'on' ? [true,'https'] : [false,'http']);
                $this->RemoteIP = $_SERVER['REMOTE_ADDR'];

                $Host = $_SERVER['HTTP_HOST'];
                $Port = (int) $_SERVER['SERVER_PORT'];
            }

            $Path = $_SERVER['DOCUMENT_URI'] ?? 'unknown';

            // @todo does anyone use username:password@ anymore?  :)
            $this->URL = new URL($Scheme,'','',hostname::str($Host),$Port,path::url($Path),encoded::arr($_GET),'');
        }
    }


    /**
     * Calculate application URLs, paths and indicators.
     *
     * The URLs are formed by merging data from a Request Struct with a base URL,
     * typically from App::$BaseURL.  This is typically called from Request::__construct.
     *
     * URL parts specified by BaseURL will overwrite those of the request.
     * BaseURL can include:
     *  - A scheme, either http:// or https://.
     *  - A hostname, or an asterisk to use the requested hostname.
     *  - A path, which is prepended to the path of the request.
     *
     * The following URL/path variables are calculated:
     *  - @c SiteURL: The base URL of the site, calculated by merging the scheme, hostname, port
     *       and path of the current request and BaseURL.  It always contains a trailing
     *       slash, is the root for all Page URLs, and used for LinkPage/Linkcnvyr URL creation.
     *  - @c MatchPath: The Path used for matching Pages, calculated by masking the BaseURL's Path from
     *       the current request.  It never has a trailing slash and is used by App::Execute to route
     *       control to a Page via PageSet::Match.
     *  - @c IsBaseScheme: TRUE if the BaseURL's scheme matches the requested scheme.
     *  - @c IsBaseHostname: TRUE if the BaseURL's hostname matches the requested hostname.
     *  - @c IsBasePort: TRUE if the BaseURL's port matches the requested port.
     *  - @c IsBasePath: TRUE if the BaseURL's path matches the requested path.
     *
     * All calculated values are stored directly in the Request array passed.
     *
     * @param array $Request A reference to a Request Struct, which is modified.
     * @param string $BaseURL Base URL to calculate from.
     *
     * @note Paths are @e not lowercased.
     * @note Username/password is not considered and not merged.
     */
    public static function CalcURLs( &$Request,$BaseURL )
    {
        $MatchPath = array();
        $IsBaseScheme = $IsBaseHostname = $IsBasePort = $IsBasePath = TRUE;

        $SiteURL = $Request;

        if( empty($BaseURL) )
        {
            $MatchPath = $SiteURL['Path'];
            $MatchPath['IsDir'] = FALSE;
            $SiteURL['Path'] = Path::Init('/');
        }
        else
        {
            /**
             * @todo what's the * for?
             */
            $BaseURL = URL::Init(str_replace('*',Hostname::ToString($SiteURL['Hostname']),$BaseURL));

            if( !empty($BaseURL['Scheme']) )
            {
                $IsBaseScheme = ($BaseURL['Scheme'] === $SiteURL['Scheme']);
                URL::SetScheme($BaseURL['Scheme'],$SiteURL);
            }

            if( !empty($BaseURL['Hostname']) )
            {
                $IsBaseHostname = ($BaseURL['Hostname'] === $SiteURL['Hostname']);
                URL::SetHostname($BaseURL['Hostname'],$SiteURL);
            }

            if( !empty($BaseURL['Port']) )
            {
                $IsBasePort = ($BaseURL['Port'] === $SiteURL['Port']);
                URL::SetPort($BaseURL['Port'],$SiteURL);
            }

            $MatchPath = $SiteURL['Path'];
            $MatchPath['IsDir'] = FALSE;

            if( $BaseURL['Path']['IsRoot'] === FALSE )
            {
                // @todo More efficient way of doing this...?
                foreach( $BaseURL['Path']['Segments'] as $K => $V )
                    $IsBasePath = isset($SiteURL['Path']['Segments'][$K]) && $SiteURL['Path']['Segments'][$K] === $V;

                URL::SetPath($BaseURL['Path'],$SiteURL);
                Path::Mask($SiteURL['Path'],$MatchPath);
            }
            else
                $SiteURL['Path'] = Path::Init('/');
        }

        $Request['SiteURL'] = $SiteURL;
        $Request['BaseURL'] = $BaseURL;
        $Request['MatchPath'] = $MatchPath;
        $Request['IsBaseScheme'] = $IsBaseScheme;
        $Request['IsBaseHostname'] = $IsBaseHostname;
        $Request['IsBasePort'] = $IsBasePort;
        $Request['IsBasePath'] = $IsBasePath;
    }


    /**
     * Read the request's full hostname, or sub-domains of it, as a string.
     *
     * A $Limit of 0 (default) returns the full hostname.  A negative
     * value returns that many sub-domains from the bottom.  A positive
     * values returns that many sub-domains from the top.
     *
     * Given @c www.asmblr.org, @c -1 returns @c www, @c 1 returns @c org,
     * @c -2 returns @c www.asmblr, @c 2 returns @c asmblr.org and
     * @c 0 (default) returns @c www.asmblr.org.
     *
     * @param int $Limit Optional number and direction of sub-domains to return.
     * @retval string The hostname string.
     *
     * @see Hostname::Top()
     * @see Hostname::Bottom()
     */
    public static function Hostname( $Limit = 0 )
    {
        if( $Limit === 0 )
            return Hostname::ToString(static::Init()['Hostname']);
        else if( $Limit < 0 )
            return Hostname::ToString(Hostname::Bottom(static::Init()['Hostname'],abs($Limit)));
        else if( $Limit > 0 )
            return Hostname::ToString(Hostname::Top(static::Init()['Hostname'],$Limit));
        else
            return 'INVALID HOSTNAME LIMIT';
    }

    /**
     * Read the request's full path or segments of it as a string.
     *
     * A $Limit of 0 (default) returns the full path.  A negative
     * value returns that many path segments from the bottom.  A positive
     * values returns that many path segments from the top.
     *
     * Given @c /first/second/third, @c -1 returns @c third, @c 1 returns @c first,
     * @c -2 returns @c /second/third, @c 2 returns @c /first/second and
     * @c 0 (default) returns @c /first/second/third.
     *
     * @param int $Limit Optional number and direction of path segments to return.
     * @retval string The path string.
     *
     * @note Paths returned using a top or bottom limit will never have leading
     *       or trailing separators (slashes).
     *
     * @see Path::Top()
     * @see Path::Bottom()
     */
    public static function Path( $Limit = 0 )
    {
        $P = static::Init()['Path'];

        if( $Limit === 0 )
        {
            return Path::ToURLString($P);
        }
        else if( $Limit < 0 )
        {
            $P['IsAbs'] = $P['IsDir'] = FALSE;
            return Path::ToURLString(Path::Bottom($P,abs($Limit)));
        }
        else if( $Limit > 0 )
        {
            $P['IsAbs'] = $P['IsDir'] = FALSE;
            return Path::ToURLString(Path::Top($P,$Limit));
        }
        else
            return 'INVALID PATH LIMIT';
    }


    /**
     * Read the request's full URL as a string.
     *
     * By default the URL returned will NOT include the query string.  Pass $_GET to have it included.
     *
     * @param array $Set A URL::Set() compatible change string/array.
     * @retval string The URL string.
     *
     * @see URL::Set()
     */
    public static function URL( $Set = array() )
    {
        if( empty($Set) )
        {
            return \asm\URL::ToString(static::Init());
        }
        else
        {
            $U = static::Init();
            \asm\URL::Set($Set,$U);
            return \asm\URL::ToString($U);
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
    public static function IsMobile()
    {
        if( !empty($_SERVER['HTTP_USER_AGENT']) )
            return (bool) preg_match('#\b(ip(hone|od)|android\b.+\bmobile|opera m(ob|in)i|windows (phone|ce)|blackberry|s(ymbian|eries60|amsung)|p(alm|rofile/midp|laystation portable)|nokia|fennec|htc[\-_]|up\.browser|[1-4][0-9]{2}x[1-4][0-9]{2})\b#i',$_SERVER['HTTP_USER_AGENT'] );
        else
            return NULL;
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


