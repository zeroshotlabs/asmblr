<?php

/**
 * Tools for common HTTP 1.1 headers and operations.
 *
 * This includes functions for common HTTP tasks, including redirecting, server errors, not found,
 * common content-types and file extensions, forcing the browser to download a file, and not caching content.
 *
 * @todo We may want the option to send useful cookies in this or another class
 *       (things like remember me, breadcrumbs/user path tracking).
 *
 * @see http://en.wikipedia.org/wiki/Internet_media_type
 * @see http://us2.php.net/manual/en/function.header.php
 */
abstract class HTTP
{
    /**
     * @var array $Types
     * Supported content types and their extensions.
     */
    public static $Types = array(
            'atom'=>'application/atom+xml',
            'rss'=>'application/rss+xml',

            'css'=>'text/css',
            'html'=>'text/html',
            'php'=>'text/x-php',
            'phtml'=>'application/x-httpd-php',

            'js'=>'text/javascript','javascript'=>'text/javascript','json'=>'application/json',
            'js4329'=>'application/javascript','javascript4329'=>'application/javascript',

            'xml'=>'text/xml',
            'app_xml'=>'application/xml',

            'text'=>'text/plain',
            'txt'=>'text/plain',
            'vcard'=>'text/vcard',
            'csv'=>'text/csv',

            'eot'=>'application/vnd.ms-fontobject','svg'=>'image/svg+xml','otf'=>'application/x-font-opentype',
            'ttf'=>'application/x-font-ttf','woff'=>'application/font-woff','woff2'=>'font/woff2',

            'gif'=>'image/gif','jpeg'=>'image/jpeg','jpg'=>'image/jpeg','png'=>'image/png',
            'ico'=>'image/x-icon','favicon'=>'image/x-icon',

            'zip'=>'application/zip','gzip'=>'application/x-gzip','gz'=>'application/x-gzip',

            'mp4'=>'video/mp4','webm'=>'video/webm',

            'pdf'=>'application/pdf',

            'excel'=>'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet',
            'xlsx'=>'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet',
            'xls'=>'application/vnd.ms-excel',

            'word'=>'application/vnd.openxmlformats-officedocument.wordprocessingml.document',
            'docx'=>'application/vnd.openxmlformats-officedocument.wordprocessingml.document',
            'doc'=>'application/msword',

            'powerpoint'=>'application/vnd.openxmlformats-officedocument.presentationml.presentation',
            'pptx'=>'application/vnd.openxmlformats-officedocument.presentationml.presentation',
            'ppt'=>'application/vnd.ms-powerpoint',

            'binary'=>'application/octet-stream',

            'form'=>'application/x-www-form-urlencoded');

    /**
     * Send a 200 OK header.
     */
    public static function _200()
    {
        header('HTTP/1.1 200 OK');
    }

    /**
     * Send a 204 No Content header (typically for OPTIONS responses).
     *
     * @param boolean $Exit FALSE to not kill execution after sending the header.
     */
    public static function _204( $Exit = TRUE )
    {
        header('HTTP/1.1 204 No Content');
        if( $Exit === TRUE )
            exit;
    }

    /**
     * Send a 301 Moved Permanently header.
     */
    public static function _301()
    {
        header('HTTP/1.1 301 Moved Permanently');
    }

    /**
     * Send a 302 Found header.
     */
    public static function _302()
    {
        header('HTTP/1.1 302 Found');
    }

    /**
     * Send a 400 Bad Request header.
     *
     * @param boolean $Exit FALSE to not kill execution after sending the header.
     */
    public static function _400( $Exit = TRUE )
    {
        header('HTTP/1.1 400 Bad Request');
        if( $Exit === TRUE )
            exit;
    }

    /**
     * Send a 401 Unauthorized header.
     *
     * @param boolean $Exit FALSE to not kill execution after sending the header.
     */
    public static function _401( $Exit = TRUE )
    {
        header('HTTP/1.1 401 Unauthorized');
        if( $Exit === TRUE )
            exit;
    }

    /**
     * Send a 403 Forbidden header.
     *
     * @param boolean $Exit FALSE to not kill execution after sending the header.
     */
    public static function _403( $Exit = TRUE )
    {
        header('HTTP/1.1 403 Forbidden');
        if( $Exit === TRUE )
            exit;
    }

    /**
     * Send a 404 Not Found header.
     */
    public static function _404()
    {
        header('HTTP/1.1 404 Not Found');
    }

    /**
     * Send a 500 Internal Server Error header.
     *
     * @param boolean $Exit FALSE to not kill execution after sending the header.
     */
    public static function _500( $Exit = TRUE )
    {
        header('HTTP/1.1 500 Internal Server Error');
        if( $Exit === TRUE )
            exit;
    }

    /**
     * Send a 501 Not Implemented header.
     *
     * @param boolean $Exit FALSE to not kill execution after sending the header.
     */
    public static function _501( $Exit = TRUE )
    {
        header('HTTP/1.1 501 Not Implemented');
        if( $Exit === TRUE )
            exit;
    }

    /**
     * Send a 503 Service Unavailable header.
     *
     * @param boolean $Exit FALSE to not kill execution after sending the header.
     */
    public static function _503( $Exit = TRUE )
    {
        header('HTTP/1.1 503 Service Unavailable');
        if( $Exit === TRUE )
            exit;
    }

    /**
     * Send a Location header for redirecting.
     *
     * @param string $URL The URL to redirect to.
     * @param URL $URL The URL Struct to redirect to.
     * @param boolean $Perm FALSE to not send a 301 header first.
     * @param boolean $Exit FALSE to not kill execution after sending the header.
     */
    public static function Location( $URL,$Perm = TRUE,$Exit = TRUE )
    {
        if( $Perm === TRUE )
            static::_301();

        header('Location: '.(is_array($URL)?URL::ToString($URL):$URL));

        if( $Exit === TRUE )
            exit;
    }

    /**
     * Send no-caching headers.
     */
    public static function NoCache()
    {
        header('Cache-Control: no-cache, must-revalidate');
        header("Expires: Sat, 1 Jan 2000 00:00:00 GMT");
    }

    /**
     * Send absolute caching headers.
     *
     * @param int $Seconds The duration, in seconds, to cache for.
     *
     * @note This is "absolute" caching - the browser won't even request the resource for the duration specified.
     *
     * @see HTTP::LastModified for conditional caching.
     * @see https://developers.google.com/speed/articles/caching
     */
    public static function Cache( $Seconds )
    {
        header('Expires: '.date('r',strtotime("+ $Seconds seconds")));
        header("Cache-Control: max-age={$Seconds}");
        header('Pragma: public');
    }

    /**
     * Send a last modified header.
     *
     * @param string $DateTime A strtotime() compatible string.
     * @param int $DateTime A Unix timestamp.
     * @param NULL $DateTime Default of NULL for current date/time.
     * @retval boolean TRUE if a valid date was determined and the header sent, FALSE if no header is sent.
     *
     * @note This is conditional caching - browser checks if resource has been modified more
     *       recently than the DateTime specified here.
     *
     * @see Cache() for absolute caching.
     */
    public static function LastModified( $DateTime = NULL )
    {
        if( $DateTime === NULL )
            $DateTime = date('r');
        else if( is_int($DateTime) === TRUE )
            $DateTime = date('r',$DateTime);
        else if( is_string($DateTime) === TRUE )
            $DateTime = @date('r',strtotime($DateTime));
        else
            $DateTime = FALSE;

        if( empty($DateTime) )
        {
            return FALSE;
        }
        else
        {
            header("Last-Modified: $DateTime");
            return TRUE;
        }
    }

    public static function RetryAfter( $Seconds = 30 )
    {
        header("Retry-After: $Seconds");
        return TRUE;
    }

    /**
     * Send a Content-Type header.
     *
     * @param string $Type Case insensitive name (extension) of a common content-type, or if it contains a forward slash it's sent as is.
     * @param string $Charset Specify the charset attribute.
     *
     * @note Some content-types can be referenced by multiple names, like a common
     *       name and file extension.  If a type isn't recognized, application/octet-stream is sent.
     * @note $Charset is passed untouched.
     *
     * @todo We should have methods for content-disposition (attachment, download, etc).
     *
     * @see $Types property for available names.
     */
    public static function ContentType( $Type,$Charset = '' )
    {
        if( $Type === Null )
            llog($_SERVER['REQUEST_URI']);
        else
            header('Content-Type: '.static::ResolveContentType($Type).(empty($Charset)?'':"; charset={$Charset}"));
    }

    /**
     * Send the headers to force a file download/save-as.
     *
     * This doesn't actually send the file.
     *
     * @param string $Filename The filename to save-as.
     * @param string $Type The content-type name.
     * @param string $Length The content-length, if known.
     *
     * @note Content length typically shouldn't be set because it causes IIS to reset the connection.
     *
     * @todo No crazy encoding of filename is done - suggestions welcome.
     * @todo No content-transfer-encoding is supported currently.
     */
    public static function SaveAs( $Filename,$Type = NULL,$Length = NULL )
    {
        static::ContentType($Type);

        header("Content-Disposition: attachment; filename=\"{$Filename}\"");
        header('Cache-Control: must-revalidate');
        header('Pragma: ');

        if( $Length !== NULL )
            header("Content-Length: $Length}");
    }

    /**
     * Send default headers to support CORS, including handling an OPTIONS request.
     *
     * This allows from any origin, GET/POST/OPTIONS methods and most headers.
     *
     * @todo This currently supports only generic default behavior.  Needs parameters to fine tune/restrict.
     *
     * @see http://enable-cors.org/server_nginx.html for nginx handling.
     * @see https://developer.mozilla.org/en-US/docs/Web/HTTP/Access_control_CORS
     */
    public static function CORS()
    {
        // support OPTIONS pre-flight with any origin and exit
        if( $_SERVER['REQUEST_METHOD'] === 'OPTIONS' )
        {
            if( !empty($_SERVER['HTTP_ORIGIN']) )
                header("Access-Control-Allow-Origin: {$_SERVER['HTTP_ORIGIN']}");

            header('Access-Control-Allow-Credentials: true');
            header('Access-Control-Max-Age: 25');    // cache for 25 seconds
            header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
            header('Access-Control-Allow-Headers: Authorization,Content-Type,Accept,Origin,User-Agent,DNT,Cache-Control,X-Mx-ReqToken,Keep-Alive,X-Requested-With,If-Modified-Since');

            header('Content-Length: 0');
            HTTP::_204();
        }
        // allow from any origin
        else if (isset($_SERVER['HTTP_ORIGIN']))
        {
            header('Access-Control-Allow-Credentials: true');
            header('Access-Control-Max-Age: 25');    // cache for 25 seconds
            header("Access-Control-Allow-Origin: {$_SERVER['HTTP_ORIGIN']}");
        }
    }

    /**
     * Resolve a content type name to it's proper MIME value, or binary (application/octet-stream)
     * if it can't be resolved.
     *
     * @param string $Type Case insensitive name (extension) of a common content-type, or if it contains a forward slash it's sent as is.
     * @param bool $Strict Return NULL instead of binary if the type can't be determined.
     * @retval NULL The content type couldn't be resolved and $Strict was set.
     * @retval string The fully qualified MIME content type.
     */
    public static function ResolveContentType( $Type,$Strict = FALSE )
    {
        if( strpos($Type,'/') !== FALSE )
        {
            return strtolower($Type);
        }
        else
        {
            $Type = strtolower($Type);
            return (isset(static::$Types[$Type])===TRUE?static::$Types[$Type]:($Strict===TRUE?NULL:static::$Types['binary']));
        }
    }

    /**
     * Return the short-names of the known content types.
     *
     * @param bool $Full TRUE to return full content types.
     * @retval array An array of known content types.
     */
    public static function GetContentTypes( $Full = FALSE )
    {
        if( $Full === TRUE )
            return static::$Types;
        else
            return array_keys(static::$Types);
    }

    /**
     * Try to determine the content type from a filename's extension.
     *
     * @param string $Filename The filename to resolve.
     * @retval string The content type.
     * @retval NULL The content type could not be determined.
     */
    public static function Filename2ContentType( $Filename )
    {
        $Ext = pathinfo($Filename);
        $Ext = strtolower(Struct::Get('extension',$Ext));

        return isset(static::$Types[$Ext])===TRUE?static::$Types[$Ext]:NULL;
    }
}


