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
 * @note This uses PHP's native streams except for POSTFiles().
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
     * @retval string Response string if non-json or $Raw is TRUE.
     * @retval array json_decode()'d data as associative array.
     *
     * @note By default, an API's response with a Content-Type of application/json will
     *       be automatically json_decode()'d.
     * @note This currently does not provide a way to change the URL's path or request headers.
     */
    public function GET( $Method,$QueryString = array(),$Raw = FALSE )
    {
        $D = array('http'=>array('method'=>'GET','follow_location'=>'5','user_agent'=>'asmblr.org restr v1'));

        $Response = file_get_contents($this($Method,$QueryString),FALSE,stream_context_create($D));

        $this->Headers = static::ParseHeaders($http_response_header);

        if( isset($this->Headers['content-type']) && strtolower($this->Headers['content-type']) === 'application/json' && $Raw === FALSE )
            return json_decode($Response,TRUE);
        else
            return $Response;
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
     * @retval string Response string if non-json or $Raw is TRUE.
     * @retval array json_decode()'d data as associative array.
     *
     * @note By default, an API's response with a Content-Type of application/json will
     *       be automatically json_decode()'d.
     * @note This currently does not provide a way to change the URL's path or request headers.
     */
    public function POST( $Method,$Payload = array(),$QueryString = array(),$Raw = FALSE )
    {
        $D = array('http'=>array('method'=>'POST','user_agent'=>'asmblr.org restr v1',
                'header'=>'Content-type: application/x-www-form-urlencoded',
                'content'=>http_build_query($Payload)));

        $Response = file_get_contents($this($Method,$QueryString),FALSE,stream_context_create($D));

        $this->Headers = static::ParseHeaders($http_response_header);

        if( isset($this->Headers['content-type']) && strtolower($this->Headers['content-type']) === 'application/json' && $Raw === FALSE )
            return json_decode($Response,TRUE);
        else
            return $Response;
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
     * @note By default, an API's response with a Content-Type of application/json will
     *       be automatically json_decode()'d.
     * @note This currently does not provide a way to change the URL's path or request headers.
     * @note CurlFile will be used if available.
     * @note The CURLError and CURLInfo arrays will be populated by curl_error()/curl_getinfo() respectively.
     */
    public function POSTFiles( $Method,$QueryString = array(),$Payload = array(),$Files = array(),$Raw = FALSE )
    {
        $CH = curl_init();

        curl_setopt($CH,CURLOPT_RETURNTRANSFER,TRUE);
        curl_setopt($CH,CURLOPT_HEADER,TRUE);
        curl_setopt($CH,CURLINFO_HEADER_OUT,TRUE);

        curl_setopt($CH,CURLOPT_SSL_VERIFYPEER,FALSE);
        curl_setopt($CH,CURLOPT_SSL_VERIFYHOST,FALSE);

        // curl_setopt($this->CH,CURLOPT_SSLCERT,fw('page')->Cert);
        // curl_setopt($this->CH,CURLOPT_SSLCERTPASSWD,fw('page')->Passwd);
        // curl_setopt($this->CH,CURLOPT_CERTINFO,TRUE);

        curl_setopt($CH,CURLOPT_HTTPHEADER,array('Expect:'));
        curl_setopt($CH,CURLOPT_USERAGENT,'asmblr.org restr v1');

        curl_setopt($CH,CURLOPT_POST,TRUE);
        curl_setopt($CH,CURLOPT_URL,$this($Method,$QueryString));

        $F = $TFiles = array();
        foreach( $Files as $K => $V )
        {
            // sort of cheesy
            if( $K[0] === '@' )
            {
                $K = trim($K,'@');
                $TFiles[$K] = tempnam(sys_get_temp_dir(),'POSTfiles');
                file_put_contents($TFiles[$K],$V);
            }

            if( class_exists('CurlFile',FALSE) )
                $F[$K] = new \CurlFile($V);
            else
                $F[$K] = '@'.trim($V);
        }

        curl_setopt($CH,CURLOPT_POSTFIELDS,array_merge($Payload,$F));

        $Response = curl_exec($CH);

        foreach( $TFiles as $T )
            unlink($T);

        if( curl_errno($CH) === 0 )
            $this->CurlError = '';
        else
            $this->CURLError = curl_error($CH);

        $this->CURLInfo = curl_getinfo($CH);

        // is this reliable?
        if( isset($this->CURLInfo['header_size']) )
        {
            $this->Headers = static::ParseHeaders(substr($Response,0,$this->CURLInfo['header_size']));
            $Response = substr($Response,$this->CURLInfo['header_size']);
        }
        else
            $this->Headers = array();

        if( isset($this->Headers['content-type']) && strtolower($this->Headers['content-type']) === 'application/json' && $Raw === FALSE )
            return json_decode($Response,TRUE);
        else
            return $Response;
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

