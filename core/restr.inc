<?php
/**
 * @file restr.php RESTful API connectivity.
 * @author @zaunere Zero Shot Labs
 * @version 5.0
 * @copyright Copyright (c) 2023 Zero Shot Laboratories, Inc. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License v3.0 or later.
 * @copyright See COPYRIGHT.txt.
 */
namespace asm\extensions\restr;


/**
 * Create URLs and communicate with a RESTish API.
 *
 * GET, POST and POST multipart (for uploading files) HTTP methods are supported.
 *
 * A BaseURL and optionally EndPoints must be defined by an extending class, or passed into the constructor.
 *
 * Endpoints are defined as key/value pairs with the following structure:
 *  @code
 *  protected $EndPoints = array('Login'=>'/login/authenticate',
 *                               'Logout'=>'/logout');
 *  @endcode
 *
 * The name of the defined endpoints are then referenced for link creation.  For example, given a
 * @c $restr object with a BaseURL of @c https://api.domain.com/v1, @c $restr('Login') would generate
 * @c https://api.domain.com/v1/login/authenticate
 *
 * @note This all uses curl - PHP's streams are unreliable, especially with HTTPS (tlsv1 alert decode error + variations between versions of openssl).
 * @note This does not enforce a valid HTTPS certificate at the destination.
 *
 * @todo Potentially add PUT and other support.
 * @todo GET/POST may not gracefully handle redirects and the headers from each.
 * @todo Enable setting of timeouts and other connection preferences, including client SSL.
 */
trait restr
// extends \asm\LinkSet implements Debuggable
{
//    use Debugged;

    /**
     * @var array $EndPoints
     * Key/value definition of API end-points: Name => /path
     *
     * @note If an endpoint's path starts with http it will be used as the absolute
     *       URL for that particular endpoint, overriding BaseURL.
     *
     * @see restr::__construct
     * @see restr::SetEndPoints
     */
    protected $EndPoints = array();

    /**
     * @var array $BasicAuth
     * Username/password array.
     *
     * @see restr::SetBasicAuth
     */
    protected $BasicAuth = array();

    /**
     * @var array $Headers
     * Key/value pairs of the response headers from the last request.
     */
    public $Headers = array();

    /**
     * @var string $CURLError
     * Error message from curl_strerror(), if any, from the last request.
     */
    public $CURLError = '';

    /**
     * @var array $CURLInfo
     * CURL info from the last request from curl_getinfo().
     */
    public $CURLInfo = array();

    /**
     * @var string $CURLResponse
     * Raw response from the last curl request.
     */
    public $CURLResponse = '';


    /**
     * Create a restr object for interacting with an API.
     *
     * @param string $BaseURL Base URL for API's endpoints.
     * @param array $EndPoints Key/value pairs of API endpoints and their paths.
     *
     * @throws Exception No restr BaseURL defined.
     *
     * @note If no endpoints are defined, the BaseURL is always returned when creating links.
    */
    public function __construct( $BaseURL = '',$EndPoints = array() )
    {
        if( !empty($BaseURL) )
            $this->SetBaseURL($BaseURL);
        else if( !empty($this->BaseURL) )
            $this->SetBaseURL($this->BaseURL);
        else
            throw new Exception('No restr BaseURL defined.');

        if( !empty($EndPoints) )
            $this->SetEndPoints($EndPoints);
    }

    /**
     * Create URLs for defined API endpoints or return the BaseURL if there's none defined.
     *
     * @param string $Name Name of the defined endpoint.
     * @param array $Set Array of change strings for one-time changes to BaseURL.
     * @retval string A well-formed URL for an endpoint.
     *
     * @note An endpoint path that starts with http will be used as the absolute URL,
     *       overriding BaseURL.
     */
    public function __invoke( $Name = NULL,$Set = array() )
    {
        $Base = $this->BaseURL;

        if( isset($this->EndPoints[$Name]) )
        {
            if( stripos($this->EndPoints[$Name],'http') === 0 )
                $Base = URL::Init($this->EndPoints[$Name]);
            else
                Path::Merge(Path::Init($this->EndPoints[$Name]),$Base['Path']);
        }

        if( !empty($Set) )
            URL::Set($Set,$Base);

        return URL::ToString($Base);
    }

    /**
     * Set the endpoints available for calls.
     *
     * @param array $EndPoints Key/value pairs of endpoints and their paths.
     */
    public function SetEndPoints( $EndPoints )
    {
        $this->EndPoints = $EndPoints;
    }

    /**
     * Make a GET request to an endpoint with optional query strings parameters.
     *
     * @param string $EndPoint The name of the defined endpoint.
     * @param array $QueryString Key/value pairs to use as a query string or change string.
     * @param string $Raw Prevent automatic JSON detection/decoding of the response.
     * @param array $Headers Headers to use in the request (key/value pairs).
     * @retval NULL Request failed.
     * @retval string Response string if non-json or $Raw is TRUE.
     * @retval array json_decode()'d data as associative array.
     *
     * @note By default, an API's response with a Content-Type of application/json or +json will be automatically json_decode()'d.
     * @note $QueryString can be used to manipulate both the path and query string using URL::Set syntax.
     */
    public function GET( $EndPoint,$QueryString = array(),$Raw = FALSE,$Headers = array() )
    {
        if( $this->curlr($this($EndPoint,$QueryString),'',$Headers) )
        {
            if( isset($this->Headers['content-type']) && $Raw === FALSE )
            {
                if( stripos($this->Headers['content-type'],'application/json') !== FALSE
                 || stripos($this->Headers['content-type'],'+json') !== FALSE )
                    return json_decode($this->CURLResponse,TRUE);
            }
            else
                return $this->CURLResponse;
        }
        else
            return NULL;
    }

    /**
     * Make a POST request to a defined endpoint with data.
     *
     * This makes a "regular" URL encoded POST. Use POSTFiles() to upload files.
     *
     * @param string $EndPoint The name of the defined endpoint.
     * @param array $Payload Key/value pairs to use as POST data.
     * @param array $QueryString Key/value pairs to use as a query string or change string.
     * @param string $Raw Prevent automatic JSON detection/decoding of the response.
     * @param array $Headers Headers to use in the request (key/value pairs).
     * @retval NULL Request failed.
     * @retval string Response string if non-json or $Raw is TRUE.
     * @retval array json_decode()'d data as associative array.
     *
     * @note By default, an API's response with a Content-Type of application/json or +json will be automatically json_decode()'d.
     * @note $QueryString can be used to manipulate both the path and query string using URL::Set syntax.
     */
    public function POST( $EndPoint,$Payload = array(),$QueryString = array(),$Raw = FALSE,$Headers = array() )
    {
        if( $this->curlr($this($EndPoint,$QueryString),$Payload,$Headers) )
        {
            if( isset($this->Headers['content-type']) && $Raw === FALSE )
            {
                if( stripos($this->Headers['content-type'],'application/json') !== FALSE
                 || stripos($this->Headers['content-type'],'+json') !== FALSE )
                    return json_decode($this->CURLResponse,TRUE);
            }
            else
                return $this->CURLResponse;
        }
        else
            return NULL;
    }

    /**
     * Make a POST multi-part request to a defined endpoint with data and optional files.
     *
     * This makes a multipart POST. Use POST() for a "regular" URL encoded POST.
     *
     * @param string $EndPoint The name of the defined endpoint.
     * @param array $QueryString Key/value pairs to use as a query string or change string.
     * @param array $Payload Key/value pairs to use as POST data.
     * @param array $Files Key/value pairs of field names and local file paths.  If a key starts with @,
     *              the string is taken as the file contents.
     * @param string $Raw Prevent automatic JSON detection/decoding of the response.
     * @retval NULL Request failed.
     * @retval string Response string if non-json or $Raw is TRUE.
     * @retval array json_decode()'d data as associative array.
     *
     * @note By default, an API's response with a Content-Type of application/json or +json will be automatically json_decode()'d.
     * @note On Windows, IUSR must be given full control permissions of c:\\windows\\temp otherwise CURL can't read the temp. files created
     *       and things will fail silently and very confusingly (file_get_contents() still can read).
     * @note This currently does not provide a way to change the request headers.
     * @note CurlFile will be used if available.
     */
    public function POSTFiles( $EndPoint,$QueryString = array(),$Payload = array(),$Files = array(),$Raw = FALSE )
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

        if( $this->curlr($this($EndPoint,$QueryString),array_merge($Payload,$F)) )
        {
            foreach( $TFiles as $T )
                unlink($T);

            if( isset($this->Headers['content-type']) && $Raw === FALSE )
            {
                if( stripos($this->Headers['content-type'],'application/json') !== FALSE
                 || stripos($this->Headers['content-type'],'+json') !== FALSE )
                    return json_decode($this->CURLResponse,TRUE);
                else
                    return $this->CURLResponse;
            }
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
     * Set a username and password for basic HTTP auth.
     *
     * @param string $Username The username.
     * @param string $Password The password.
     */
    public function SetBasicAuth( $Username,$Password )
    {
        $this->BasicAuth = array($Username,$Password);
    }

    /**
     * Make request using curl.
     *
     * This method is used internally to actually perform the HTTP request using curl.
     *
     * @param string $URL The URL to make the request to.
     * @param array $POSTPayload Key/value pairs of POST data; will be multipart encoded.
     * @param string $POSTPayload Encoded string of POST data; will be form-urlencoded.
     * @param array $Headers Headers to use in the request (key/value pairs).
     * @param string $Verb Custom HTTP verb to use.  Setting to PUT will use POSTPayload as payload string.
     * @retval boolean TRUE upon successful request.
     *
     * @note If POSTPayload is empty, a GET will be performed.
     * @note If POSTPayload is a string, form-urlencoded; array = multipart  (unless overriden by a header)
     * @note If Verb is PUT POSTPayload is taken as a string of data to upload (binary).
     * @todo Determine behavior of $Headers if non-empty.  Will they be the only headers sent, or are others automagically added?
     * @todo Add proper DebugOn/DebugOff handling.
     */
    protected function curlr( $URL,$POSTPayload = array(),$Headers = array(),$Verb = '' )
    {
        // :)
        if( isset($_SERVER[$this->DebugToken]) )
            $Debug = TRUE;

        $CH = curl_init();

        curl_setopt($CH,CURLOPT_RETURNTRANSFER,TRUE);
        curl_setopt($CH,CURLOPT_HEADER,TRUE);
        curl_setopt($CH,CURLOPT_FOLLOWLOCATION,TRUE);

        curl_setopt($CH,CURLOPT_SSL_VERIFYPEER,FALSE);
        curl_setopt($CH,CURLOPT_SSL_VERIFYHOST,FALSE);

        if( !empty($this->BasicAuth) )
        {
            curl_setopt($CH,CURLOPT_HTTPAUTH,CURLAUTH_BASIC);
            curl_setopt($CH,CURLOPT_USERPWD,"{$this->BasicAuth[0]}:{$this->BasicAuth[1]}");
        }

        // curl_setopt($this->CH,CURLOPT_SSLCERT,$Cert);
        // curl_setopt($this->CH,CURLOPT_SSLCERTPASSWD,$Passwd);
        // curl_setopt($this->CH,CURLOPT_CERTINFO,TRUE);

        // may get overwritten per headers...?
        curl_setopt($CH,CURLOPT_USERAGENT,'asmblr.org restr v1.1');

        // setup doing a custom request, like PUT
        if( !empty($Verb) )
        {
            $Verb = strtoupper(trim($Verb));

            curl_setopt($CH,CURLOPT_CUSTOMREQUEST,$Verb);

            if( $Verb === 'PUT' )
                curl_setopt($CH,CURLOPT_POSTFIELDS,$POSTPayload);
        }
        // a POST - automatically sets correct content-type unless override below
        // otherwise will be a GET by default
        else if( !empty($POSTPayload) )
        {
            curl_setopt($CH,CURLOPT_POSTFIELDS,$POSTPayload);
        }

        // remove Expect unless explicitly specified
        if( !isset($Headers['Expect']) )
            $Headers['Expect'] = '';

        $CURLHeaders = array();
        foreach( $Headers as $K => $V )
        {
            // encoding?
            $CURLHeaders[] = "{$K}: {$V}";
        }

        curl_setopt($CH,CURLOPT_HTTPHEADER,$CURLHeaders);

        curl_setopt($CH,CURLOPT_URL,$URL);

        if( !empty($Debug) )
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

        if( !empty($Debug) )
        {
            llog(\asm\Debug::Dump(curl_getinfo($CH)));
            llog('RESPONSE HEADERS');
            llog(\asm\Debug::Dump($this->Headers));
            llog($this->CURLResponse);
        }

        return TRUE;
    }

    /**
     * Dump debug details about the last request/response.
     */
    public function curlrDump()
    {
        var_dump($this->Headers);
        var_dump($this->CURLError);
        var_dump($this->CURLInfo);
        var_dump($this->CURLResponse);
    }

    /**
     * Helper method for parsing $http_response_header array (PHP streams) or a string (CURL) into an
     * associative array.  The HTTP response header will be available in the 'http' element.
     *
     * @param array $Raw $http_response_header from a PHP streams call.
     * @param string $Raw Headers string from a curl call.
     * @retval array Associative array of headers/values.
     *
     * @note Multiple headers of the same name will overwrite.
     * @note All header names are lowercased.
     * @todo Assumes \\r\\n if a string of headers.
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
                $K = explode(':',$V,2);
                $Headers[strtolower(trim($K[0]))] = trim($K[1]);
            }
        }

        return $Headers;
    }
}

