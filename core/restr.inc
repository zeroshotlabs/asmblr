<?php
/**
 * @file cnvyr.inc cnvyr asset/media delivery and manipulation.
 * @author Stackware, LLC
 * @version 4.0
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * Create URLs and communicate with a RESTish API.
 *
 * GET, POST and POST multipart (for uploading files) methods are supported.
 *
 * Methods and a BaseURL must be defined by an extending class, or passed into the constructor.
 *
 * The name of the defined methods is then used for link creation.
 *
 * @note This all uses curl - PHP's streams are unreliable, especially with HTTPS (tlsv1 alert decode error + variations between versions of openssl).
 * @note This does not enforce a valid HTTPS certificate at the destination.
 * @todo Potentially add PUT and other support.
 * @todo GET/POST may not gracefully handle redirects and the headers from each.
 * @todo Enable setting of timeouts and other connection preferences, including client SSL.
 */
class restr extends \asm\LinkSet
{
    /**
     * Key/value pairs of methods and their path, such as:
     *
     * 'Login'=>'/login/authenticate'
     *
     * Given an object $restr with a BaseURL of https://api.domain.com
     * $rest('Login') would generate https://api.domain.com/login/authenticate
     */
    protected $Methods = array();

    /**
     * Key/value pairs of the response headers of the last request.
    */
    public $Headers = array();

    /**
     * Info from curl_error()/curl_getinfo(), respectively, populated by
     * POSTFiles().
    */
    public $CURLError = '';
    public $CURLInfo = array();

    /**
     * Raw response from the curl request.
     */
    public $CURLResponse = '';


    /**
     * Create a restr object for interacting with an API.
     *
     * @param string $BaseURL Base URL for API's methods.
     * @param array $Methods Key/value pairs of methods and their path.
     *
     * @throws Exception No restr BaseURL defined.
     * @throws Exception No restr Methods defined.
    */
    public function __construct( $BaseURL = '',$Methods = array() )
    {
        if( !empty($BaseURL) )
            $this->SetBaseURL($BaseURL);
        else if( !empty($this->BaseURL) )
            $this->SetBaseURL($this->BaseURL);
        else
            throw new Exception('No restr BaseURL defined.');

        if( !empty($Methods) )
            $this->Methods = $Methods;
        else if( empty($this->Methods) )
            throw new Exception('No restr Methods defined.');
    }

    /**
     * Create URLs for defined API methods.
     *
     * @param string $Name Name of the defined method.
     * @see \asm\LinkSet::__invoke() for details.
     */
    public function __invoke( $Name = NULL,$Set = array() )
    {
        if( isset($this->Methods[$Name]) )
        {
            $Base = $this->BaseURL;
            Path::Merge(Path::Init($this->Methods[$Name]),$Base['Path']);

            if( !empty($Set) )
                URL::Set($Set,$Base);

            return URL::ToString($Base);
        }
        else
            return 'not-found';
    }

    /**
     * Make a GET request to a Method with optional query strings parameters.
     *
     * @param string $Method The name of the defined method.
     * @param array $QueryString Key/value pairs to use as a query string.
     * @param string $Raw Prevent automatic JSON detection/decoding of the response.
     * @param array $Headers Headers to use in the request (key/value pairs).
     * @retval null Request failed.
     * @retval string Response string if non-json or $Raw is TRUE.
     * @retval array json_decode()'d data as associative array.
     *
     * @todo Check that setting $Headers will make them the only headers sent in the request.  Same in the other methods.
     * @note By default, an API's response with a Content-Type of application/json will
     *       be automatically json_decode()'d.
     * @note This currently does not provide a way to change the URL's path.
     */
    public function GET( $Method,$QueryString = array(),$Raw = FALSE,$Headers = array() )
    {
        if( $this->curlr($this($Method,$QueryString),'',$Headers) )
        {
            if( isset($this->Headers['content-type']) && strtolower($this->Headers['content-type']) === 'application/json' && $Raw === FALSE )
                return json_decode($this->CURLResponse,TRUE);
            else
                return $this->CURLResponse;
        }
        else
            return NULL;
    }

    /**
     * Make a POST request to a defined Method with data.
     *
     * This makes a "regular" URL encoded POST.  Use POSTFiles() to upload files.
     *
     * @param string $Method The name of the defined method.
     * @param array $Payload Key/value pairs to use as POST data.
     * @param array $QueryString Key/value pairs to use as a query string.
     * @param string $Raw Prevent automatic JSON detection/decoding of the response.
     * @param array $Headers Headers to use in the request (key/value pairs).
     * @retval null Request failed.
     * @retval string Response string if non-json or $Raw is TRUE.
     * @retval array json_decode()'d data as associative array.
     *
     * @note By default, an API's response with a Content-Type of application/json will
     *       be automatically json_decode()'d.
     * @note This currently does not provide a way to change the URL's path or request headers.
     */
    public function POST( $Method,$Payload = array(),$QueryString = array(),$Raw = FALSE,$Headers = array() )
    {
        if( $this->curlr($this($Method,$QueryString),$Payload,$Headers) )
        {
            if( isset($this->Headers['content-type']) && strtolower($this->Headers['content-type']) === 'application/json' && $Raw === FALSE )
                return json_decode($this->CURLResponse,TRUE);
            else
                return $this->CURLResponse;
        }
        else
            return NULL;
    }

    /**
     * Make a POST multi-part request to a defined Method with data and optional files.
     *
     * This makes a multipart POST.  Use POST() for a "regular" URL encoded POST.
     *
     * @param string $Method The name of the defined method.
     * @param array $QueryString Key/value pairs to use as a query string.
     * @param array $Payload Key/value pairs to use as POST data.
     * @param array $Files Key/value pairs of field names and local file paths.  If a key starts with @,
     *        the string is taken as the file contents.
     * @param string $Raw Prevent automatic JSON detection/decoding of the response.
     * @retval string Response string if non-json or $Raw is TRUE.
     * @retval array json_decode()'d data as associative array.
     *
     * @todo Add request header overrides per the other methods.
     * @note By default, an API's response with a Content-Type of application/json will
     *       be automatically json_decode()'d.
     * @note On Windows, IUSR must be given full control permissions of c:\windows\temp otherwise CURL can't
     *       properly read the temp. files create.
     * @note This currently does not provide a way to change the URL's path or request headers.
     * @note CurlFile will be used if available.
     * @note The CURLError and CURLInfo arrays will be populated by curl_error()/curl_getinfo() respectively.
     */
    public function POSTFiles( $Method,$QueryString = array(),$Payload = array(),$Files = array(),$Raw = FALSE )
    {
        $F = $TFiles = array();
        foreach( $Files as $K => $V )
        {
            // sort of cheesy - indicates content itself as string, rather than a filename so write to a temp. location
            if( $K[0] === '@' )
            {
                $K = trim($K,'@');
                $T = $TFiles[$K] = tempnam(sys_get_temp_dir(),'POSTfiles');
                file_put_contents($TFiles[$K],$V);
            }
            else
                $T = $V;

            if( class_exists('CurlFile',FALSE) )
                $F[$K] = new \CurlFile($T);
            else
                $F[$K] = '@'.trim($T);
        }

        if( $this->curlr($this($Method,$QueryString),array_merge($Payload,$F)) )
        {
            foreach( $TFiles as $T )
                unlink($T);

            if( isset($this->Headers['content-type']) && strtolower($this->Headers['content-type']) === 'application/json' && $Raw === FALSE )
                return json_decode($this->CURLResponse,TRUE);
            else
                return $this->CURLResponse;
        }
        else
        {
            foreach( $TFiles as $T )
                unlink($T);

            return NULL;
        }
    }

    /**
     * Make request using curl.
     *
     * @param string $URL The URL to make the request to.
     * @param array $POSTPayload Key/value pairs of POST data; will be multipart encoded.
     * @param string $POSTPayload Encoded string of POST data; will be form-urlencoded.
     * @param array $Headers Headers to use in the request (key/value pairs).
     * @return boolean TRUE upon successful request.
     * 
     * @note If POSTPayload is empty, a GET will be performed.
     * @note If POSTPayload is a string, form-urlencoded; array = multipart  (unless overriden by a header)
     * @todo Determine behavior of $Headers per comments in GET - right now things seem
     *       to be correct and overwritten as needed.
     */
    protected function curlr( $URL,$POSTPayload = array(),$Headers = array() )
    {
        // :)  should be using DebugOn/etc
        $Debug = FALSE;
        
        $CH = curl_init();

        curl_setopt($CH,CURLOPT_RETURNTRANSFER,TRUE);
        curl_setopt($CH,CURLOPT_HEADER,TRUE);
        curl_setopt($CH,CURLOPT_FOLLOWLOCATION,TRUE);

        curl_setopt($CH,CURLOPT_SSL_VERIFYPEER,FALSE);
        curl_setopt($CH,CURLOPT_SSL_VERIFYHOST,FALSE);

        // curl_setopt($this->CH,CURLOPT_SSLCERT,$Cert);
        // curl_setopt($this->CH,CURLOPT_SSLCERTPASSWD,$Passwd);
        // curl_setopt($this->CH,CURLOPT_CERTINFO,TRUE);

        // may get overwritten per headers
        curl_setopt($CH,CURLOPT_USERAGENT,'asmblr.org restr v1');

        // automatically sets correct content-type unless override below
        if( !empty($POSTPayload) )
            curl_setopt($CH,CURLOPT_POSTFIELDS,$POSTPayload);

        // for now we always remove Expect:
        $CURLHeaders = array('Expect:');

        foreach( $Headers as $K => $V )
        {
            // encoding?
            $CURLHeaders[] = "{$K}: {$V}";
        }

        curl_setopt($CH,CURLOPT_HTTPHEADER,$CURLHeaders);

        curl_setopt($CH,CURLOPT_URL,$URL);

        if( $Debug )
        {
            curl_setopt($CH,CURLINFO_HEADER_OUT,TRUE);
            curl_setopt($CH,CURLOPT_VERBOSE,TRUE);  // doesn't seem to actually do anything
        }

        $Response = curl_exec($CH);

        if( curl_errno($CH) === 0 )
        {
            $this->CURLError = '';
        }
        else
        {
            $this->CURLError = curl_strerror(curl_errno($CH));
            return FALSE;
        }

        $this->CURLInfo = curl_getinfo($CH);

        // is this reliable?
        if( isset($this->CURLInfo['header_size']) )
        {
            $this->Headers = static::ParseHeaders(substr($Response,0,$this->CURLInfo['header_size']));
            $this->CURLResponse = substr($Response,$this->CURLInfo['header_size']);
        }
        else
            $this->Headers = array();

        if( $Debug )
        {
            llog(\asm\Debug::Dump(curl_getinfo($CH)));
            llog('RESPONSE HEADERS');
            llog(\asm\Debug::Dump($this->Headers));
        }
        
        return TRUE;
    }

    /**
     * Helper method for parsing $http_response_header (PHP streams) or a string (CURL) into an
     * associative array.  The full HTTP response header will be available in the 'http' element.
     *
     * @param array $Raw $http_response_header from a PHP streams call.
     * @retval array Associative array of headers/values.
     *
     * @note Multiple headers of the same name will overwrite.
     * @note All header names are lowercased.
     * @todo Assumes \r\n if a string of headers.
     */
    public static function ParseHeaders( $Raw )
    {
        if( is_string($Raw) )
            $Raw = explode("\r\n",trim($Raw));

        $Headers = array();
        foreach( $Raw as $V )
        {
            if( strpos($V,'HTTP/') === 0 )
            {
                $Headers['http'] = $V;
            }
            else if( empty($V) )
            {
                continue;
            }
            else
            {
                $K = explode(':',$V);
                $Headers[strtolower(trim($K[0]))] = trim($K[1]);
            }
        }

        return $Headers;
    }
}

