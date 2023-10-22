<?php declare(strict_types=1);
/**
 * @file http.php Provides HTTP headers and utilities such as saving/uploading files.
 * @author @zaunere Zero Shot Labs
 * @version 5.0
 * @copyright Copyright (c) 2023 Zero Shot Laboratories, Inc. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License v3.0 or later.
 * @copyright See COPYRIGHT.txt.
 */
namespace asm\http;
use function \asm\sys\mime_by_name;


/**
 * Tools for common HTTP 1.1 headers and operations.
 *
 * This includes functions for common HTTP tasks, including redirecting,
 * server errors, not found, sending common content-types based on file extensions,
 * forcing the browser to download a file, and alterating caching behavior.
 *
 * @todo May want the option to send useful cookies in this or another class
 *       (things like remember me, breadcrumbs/user path tracking).
 * 
 * @note Known when to send them, know when to hold 'em.
 * @note In cases where header() isn't needed, http_response_code() is used.
 *
 * @see http://en.wikipedia.org/wiki/Internet_media_type
 * @see http://us2.php.net/manual/en/function.header.php
 * @see asm\_e\eHTTP
 */
trait http_headers
{
    public function send_response_code( int $code ): int
    {
        return http_response_code($code);
    }

    /**
     * Send no-caching headers (Expires).
     */
    public function expired(): void
    {
        header('Cache-Control: no-cache, must-revalidate');
        header("Expires: Sat, 1 Jan 2000 00:00:00 GMT");
    }

    /**
     * Send absolute caching headers (Expires, Cache-Control, Pragma).
     *
     * @param int $secs The duration, in seconds, to cache for.
     *
     * @note This is "absolute" caching - the browser won't attempt a request of
     *       the resource for the duration specified.
     *
     * @see http_headers::last_modified() for conditional caching.
     * @see https://developers.google.com/speed/articles/caching
     */
    public function abs_cache( $secs ): void
    {
        header('Expires: '.date('r',strtotime("+ $secs seconds")));
        header("Cache-Control: max-age={$secs}");
        header('Pragma: public');
    }

    /**
     * Send conditional caching headers (LastModified).
     *
     * @param string $datetime A strtotime() compatible string to indicate the Last Modified date.
     * @param int $datetime A Unix timestamp to indicate the Last Modified date.
     * @param NULL $datetime Default of NULL for current date/time.
     * @return boolean True if a valid date was determined and the header sent,
     *                 false if a date couldn't be determined and no header was sent.
     *
     * @note This is conditional caching - browser checks if resource has been
     *       modified more recently than the datetime specified here.
     *
     * @see http_headers::abs_cache() for absolute caching.
     */
    public function last_modified( $datetime = null ): bool
    {
        if( $datetime === null )
            $datetime = date('r');
        else if( is_int($datetime) === true )
            $datetime = date('r',(int)$datetime);
        else if( is_string($datetime) === true )
            $datetime = @date('r',strtotime($datetime));
        else
            $datetime = false;

        if( !empty($datetime) )
        {
            header("Last-Modified: $datetime");
            return true;
        }
        else
            return false;
    }

    /**
     * Send a Retry-After header.
     *
     * @param int $secs The number of seconds to wait before retrying.
     *
     * @note This is used for rate limiting and commonly with 503, 429 or redirects.
     * @see https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Retry-After
     */
    public function retry_after( $secs = 30 ): void
    {
        header("Retry-After: $secs");
    }

    /**
     * Send a Content-Type header.
     *
     * @param string $Type Case insensitive name (extension) of a common content-type, or if it contains a forward slash it's sent as is.
     * @param string $Charset Specify the charset attribute.
     *
     * @note Some content-types can be referenced by multiple names, like a common
     *       name and file extension.  If a type isn't recognized, the original value is returned.
     * @note $Charset is passed untouched.
     *
     * Sends a Content-Type header. 
     * 
     * This expects a valid content type value.  Use mime_by_name to get a valid type for
     * common extensions and application names.
     * 
     * @todo We should have methods for content-disposition (attachment, download, etc).
     *
     * @see mime_by_name() to get a valid MIME type for an extension.
     */
    public function content_type( string $type,$charset = '' ): void
    {
        if( !empty($type) )
            header("Content-Type: $type".(empty($charset)?'':"; charset={$charset}"));
    }

    public function content_length( int $length ): void
    {
        header("Content-Length: $length");
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
    public function save_as( $filename,$type = null,$length = null )
    {
        $this->content_type($type);

        header("Content-Disposition: attachment; filename=\"{$filename}\"");
        header('Cache-Control: must-revalidate');
        header('Pragma: ');

        if( $length !== null )
            header("Content-Length: $length}");
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
// abstract class FileUpload
// {
//     protected static $Skel = array('Filename'=>'','ContentType'=>'','TmpPath'=>'',
//                                    'Error'=>0,'FileSize'=>0);

//     /**
//      * @var array $Err2Txt
//      * Error code constants to text mappings.
//      */
//     protected static $Err2Txt = array(UPLOAD_ERR_OK=>'Ok',UPLOAD_ERR_INI_SIZE=>'Upload too big (UPLOAD_ERR_INI_SIZE)',
//                                       UPLOAD_ERR_FORM_SIZE=>'Upload too big (UPLOAD_ERR_FORM_SIZE)',
//                                       UPLOAD_ERR_PARTIAL=>'Partial upload (UPLOAD_ERR_PARTIAL)',
//                                       UPLOAD_ERR_NO_FILE=>'No file upload (UPLOAD_ERR_NO_FILE)',
//                                       UPLOAD_ERR_NO_TMP_DIR=>'Missing temp. directory (UPLOAD_ERR_NO_TMP_DIR)',
//                                       UPLOAD_ERR_CANT_WRITE=>'Failed to write to disk (UPLOAD_ERR_CANT_WRITE)',
//                                       UPLOAD_ERR_EXTENSION=>'A PHP extension stopped the upload (UPLOAD_ERR_EXTENSION)');

//     /**
//      * Get data about the current request's uploaded files.
//      *
//      * The $_FILES superglobal is used automatically.  For multiple file
//      * uploads, the name of the form field is assumed to be of the
//      * FieldName[] naming convention.
//      *
//      * @param string $Name The name of the file upload form field or empty to take all of $_FILES.
//      * @retval array An array of one or more FileUpload Structs, or an empty array if no files have been uploaded.
//      */
//     public static function Init( $Name )
//     {
//         if( empty($_FILES) )
//             return array();

//         $Files = array();

//         // multiple files, each with a different name (none [] syntax)
//         if( empty($Name) )
//         {
//             foreach( $_FILES as $K => $V )
//             {
//                 if( $V['error'] === UPLOAD_ERR_NO_FILE )
//                     continue;

//                 $F = static::$Skel;
//                 $F['Filename'] = $V['name'];
//                 $F['ContentType'] = $V['type'];
//                 $F['TmpPath'] = $V['tmp_name'];
//                 $F['Error'] = $V['error'];
//                 $F['FileSize'] = $V['size'];
//                 $Files[$K] = $F;
//             }
//         }
//         // Multiple file uploads
//         else if( is_array($_FILES[$Name]['name']) )
//         {
//             foreach( $_FILES[$Name]['name'] as $K => $V )
//             {
//                 if( $_FILES[$Name]['error'][$K] === UPLOAD_ERR_NO_FILE )
//                     continue;

//                 $F = static::$Skel;
//                 $F['Filename'] = $_FILES[$Name]['name'][$K];
//                 $F['ContentType'] = $_FILES[$Name]['type'][$K];
//                 $F['TmpPath'] = $_FILES[$Name]['tmp_name'][$K];
//                 $F['Error'] = $_FILES[$Name]['error'][$K];
//                 $F['FileSize'] = $_FILES[$Name]['size'][$K];
//                 $Files[] = $F;
//             }
//         }
//         // Single file upload
//         else
//         {
//             if( $_FILES[$Name]['error'] === UPLOAD_ERR_NO_FILE )
//                 return array();

//             $F = static::$Skel;
//             $F['Filename'] = $_FILES[$Name]['name'];
//             $F['ContentType'] = $_FILES[$Name]['type'];
//             $F['TmpPath'] = $_FILES[$Name]['tmp_name'];
//             $F['Error'] = $_FILES[$Name]['error'];
//             $F['FileSize'] = $_FILES[$Name]['size'];
//             $Files[] = $F;
//         }

//         return $Files;
//     }

//     /**
//      * Check if a particular FileUpload Struct is free from upload errors.
//      *
//      * @param array $File FileUpload Struct to check.
//      * @retval boolean TRUE if the file upload was successful.
//      */
//     public static function IsOK( $File )
//     {
//         return (isset($File['Error']) && $File['Error'] === UPLOAD_ERR_OK)?TRUE:FALSE;
//     }

//     /**
//      * Return an error message for one of PHP's file upload error constants.
//      *
//      * @param int $Err The PHP error constant.
//      * @retval string The error message or Unknown.
//      */
//     public static function Err2Txt( $Err )
//     {
//         if( isset(static::$Err2Txt[$Err]) )
//             return static::$Err2Txt[$Err];
//         else
//             return 'Unknown';
//     }
// }

