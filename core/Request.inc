<?php
/**
 * @file Request.inc Request data and management.
 * @author Stackware, LLC
 * @version 4.0
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * Current request data.
 *
 * The Request Struct encapsulates the raw request data for both HTTP and CLI requests.
 *
 * It is used by App to calculate application URLs and determine whether the request
 * came from a web browser or from the command line, and can be used throughout an
 * application to access information about the current request.
 *
 * $_GET and $_POST data is not included.
 *
 * The Struct is a "singleton" - once it's been initialized, it's values are persisted
 * in the $Request static variable.
 *
 * @note Request can be used as a URL.
 */
abstract class Request extends Struct
{
    /**
     * Same as URL with addition of IsCLI.
     */
    protected static $Skel = array('IsHTTPS'=>FALSE,'Scheme'=>'','Username'=>'','Password'=>'',
    							   'Hostname'=>array(),'Port'=>'','Path'=>array(),'Query'=>array(),'Fragment'=>'',
                                   'IsCLI'=>FALSE);

    /**
     * Current request data.
     */
    protected static $Request = NULL;


    /**
     * Get the current request's data.
     *
     * Contains actual request data, normalized from $_SERVER.  No processing is performed
     * except to cleanup the path.
     *
     * This detects whether a request appears to be coming from a web server or CLI.
     *
     * For CLI:
     *  - Hostname is set to the value of gethostname().
     *  - Path is set to the value of the first command line argument (argv[1]).
     *
     * @todo Use of PHP_SAPI?
     * @todo Allow forging a request, i.e. pass in a URL and return the result, rather than store statically.
     */
    public static function Init()
    {
        if( static::$Request !== NULL )
            return static::$Request;

        $Request = static::$Skel;

        // we're running as a CLI
        if( !empty($_SERVER['argv']) )
        {
            $Request['IsCLI'] = TRUE;
            $Request['Hostname'] = Hostname::Init(gethostname());

            if( !empty($_SERVER['argv'][1]) )
                $Request['Path'] = Path::Init($_SERVER['argv'][1]);
            else
                exit(PHP_EOL.'Page URL not provided via command line.'.PHP_EOL.PHP_EOL);
        }
        else
        {
            $Request['Scheme'] = (empty($_SERVER['HTTPS'])||$_SERVER['HTTPS']==='off')?'http':'https';

            // If the hostname can't be determined, it's now silently left empty
            if( !empty($_SERVER['HTTP_HOST']) )
                $Request['Hostname'] = Hostname::Init($_SERVER['HTTP_HOST']);
            else if( !empty($_SERVER['SERVER_NAME']) )
                $Request['Hostname'] = Hostname::Init($_SERVER['SERVER_NAME']);

            $Request['Username'] = (string)static::Get('PHP_AUTH_USER',$_SERVER);
            $Request['Password'] = (string)static::Get('PHP_AUTH_PW',$_SERVER);

            if( !empty($_SERVER['SERVER_PORT']) && ($_SERVER['SERVER_PORT'] !== '80') && ($_SERVER['SERVER_PORT'] !== '443') )
                $Request['Port'] = $_SERVER['SERVER_PORT'];

            if( empty($_SERVER['QUERY_STRING']) )
            {
                // no request and no query means we're likely being incorrectly executed via command line directly using the php-cgi binary
                $Request['Path'] = Path::Init(empty($_SERVER['REQUEST_URI'])?'':rtrim(urldecode($_SERVER['REQUEST_URI']),'?'));
            }
            else
            {
                $Request['Path'] = Path::Init(urldecode(substr($_SERVER['REQUEST_URI'],0,strpos($_SERVER['REQUEST_URI'],'?'))));
            }
        }

        return (static::$Request = $Request);
    }

    /**
     * Calculate application URLs.
     *
     * The URLs are formed by merging data from a Request Struct with a base URL,
     * typically from App::$BaseURL.
     *
     * URL parts specified by BaseURL will overwrite those of the request.
     * BaseURL can include:
     *  - A scheme, either http:// or https://.
     *  - A hostname, or an asterisk to use the requested hostname.
     *  - A path, which is prepended to any requested path.
     *
     * The following URL variables are created:
     *  - SiteURL: The base URL of the site, calculated by merging the scheme, hostname, port
     *             and path of the current request and BaseURL.  It always contains a trailing
     *             slash, is the root for all Page requests, and used for LinkSet URL creation,
     *             among other things.
     *  - MatchPath: The path used for matching Pages, calculated by removing a Path from
     *               BaseURL from the current request, never has a trailing slash, and used
     *               for matching Pages in PageSet::Match().
     *
     * The calculated Path used for matching Pages (Path Struct).
    public $MatchPath = array();

     * The calculated base URL of the site (URL Struct).
    public $SiteURL = array();

    base URL struct as configured
    public $BaseURL

     * TRUE if the BaseURL's Scheme matched our request's.
    public $IsBaseScheme = TRUE;

     * TRUE if the BaseURL's Hostname matched our request's.
    public $IsBaseHostname = TRUE;

     * TRUE if the BaseURL's Port matched our request's.
    public $IsBasePort = TRUE;

     * TRUE if the BaseURL's Path matched our request's.
    public $IsBasePath = TRUE;
     *
     *
     * Additionally, flags about how closely the request matched the base URL are
     * calculated.
     *
     * All calculated values are stored directly in the Request array passed.
     *
     * @param array $Request A reference to a Request Struct which is modified.
     * @param string $BaseURL Base URL to calculate from.
     *
     * @note Paths are lowercased.
     * @note Username/password is not considered and not merged.
     */
    public static function CalcURLs( &$Request,$BaseURL )
    {
        $MatchPath = array();
        $IsBaseScheme = $IsBaseHostname = $IsBasePort = $IsBasePath = TRUE;

        $SiteURL = $Request;

        Path::Lower($SiteURL['Path']);

        if( empty($BaseURL) )
        {
            $MatchPath = $SiteURL['Path'];
            $MatchPath['IsDir'] = FALSE;
            $SiteURL['Path'] = Path::Init('/');
        }
        else
        {
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

//        return array($SiteURL,$MatchPath,$IsBaseScheme,$IsBaseHostname,$IsBasePort,$IsBasePath);
    }




    /**
     * Read the request's full hostname or sub-domains of it as a string.
     *
     * A $Limit of 0 (default) returns the full hostname.  A negative
     * value returns that many sub-domains from the bottom.  A positive
     * values returns that many sub-domains from the top.
     *
     * Given www.framewire.org, -1 returns \c www, 1 returns \c org,
     * -2 returns \c www.framewire, 2 returns \c framewire.org and
     * 0 (default) returns \c www.framewire.org.
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
     * Given /first/second/third, -1 returns \c third, 1 returns \c first,
     * -2 returns \c /second/third, 2 returns \c /first/second and
     * 0 (default) returns \c /first/second/third.
     *
     * @param int $Limit Optional number and direction of path segments to return.
     * @retval string The path string.
     *
     * @note Paths returned using a top or bottom limit will never have leading
     * or trailing separators (slashes).
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
     * Determine if the request is ChromePHP aware.
     *
     * @note ChromePHP doesn't currently announce itself so this has little use
     *       https://github.com/ccampbell/chromelogger/issues/8
     * @note Set fwApp::$LogPublic to TRUE/FALSE until they fix this.
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
abstract class FileUpload extends Struct
{
    protected static $Skel = array('Filename'=>'','ContentType'=>'','TmpPath'=>'',
                                   'Error'=>0,'FileSize'=>0);

    /**
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
     * @param string $Name The name of the file upload form field.
     * @retval array An array of one or more FileUpload Structs, or an
     *               empty array if no files have been uploaded.
     */
    public static function Init( $Name )
    {
        if( empty($_FILES) )
            return array();

        $Files = array();

        // Multiple file uploads
        if( is_array($_FILES[$Name]['name']) )
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


