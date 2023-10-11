<?php declare(strict_types=1);
/**
 * @file sys.php Extension loading, debugging and error handling.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm\sys;
use asm\_e\e500;
use phpDocumentor\Descriptor\Tag\VersionDescriptor;

/**
 * Determine whether we're executing in a Windows environment.
 *
 * @return bool TRUE if the application is executing in a Windows environment.
 */
function IsWindows()
{
    return isset($_SERVER['SystemRoot']);
}


/**
 * Load an extension that's bundled with asmblr.
 *
 * Extensions are subdirectories in extensions/ each with a load.inc file.
 * 
 * It is case-sensitive.
 *
 * @param string $ExtLoader The extension's name (directory).
 * @throws Exception Extension '$ext' not found.
 *
 * @note This uses require() so pay attention.
 * @note Improve path handling - ASM_EXT_ROOT is hardwired currently.
 */
function load_ext( $ext )
{
    if( is_file(ASM_EXT_ROOT.$ext.DIRECTORY_SEPARATOR."load.inc") )
        require(ASM_EXT_ROOT.$ext.DIRECTORY_SEPARATOR."load.inc");
    else
        throw new e500("Extension '$ext' not found.");
}


/**
 * Output directly to stdout.
 * 
 * If running as a web app, this will output directly to the browser.
 * If running as a CLI, this will output directly to the console.
 * 
 * @param string $msg The message to output.
 * @param int $pre_lines Number of blank lines to output before the message.
 * @param int $post_lines Number of blank lines to output after the message.
 * 
 * @note Use this carefully as it doesn't check whether the app is in
 * production or not, thus raw display of messages to the user.  Best used
 * with a CLI app.
 * 
 * @todo Add coloring.
 */
function _stdo( $msg,$pre_lines = 1,$post_lines = 2 )
{
    $pre_lines = str_repeat(PHP_EOL,$pre_lines);
    $post_lines = str_repeat(PHP_EOL,$post_lines);

    fwrite($GLOBALS['STDOUT']??STDOUT,"{$pre_lines}{$msg}{$post_lines}");
}


/**
 * Output directly to stderr, using error_log().
 * 
 * If running as a web app, by default this will output to the server error log.
 * If running as a CLI, this will output directly to STDERR on the console, by default.
 * 
 * @param string $msg The message to output.
 * @param int $pre_lines Number of blank lines to output before the message.
 * @param int $post_lines Number of blank lines to output after the message.
 * 
 * @note This is safe to use in production, assuming display_errors or something
 * stupid is set to TRUE.
 * 
 * @todo need to add set_error_handler() sometime.
 */
function _stde( $msg,$pre_lines = 1,$post_lines = 2 )
{
    $pre_lines = str_repeat(PHP_EOL,$pre_lines);
    $post_lines = str_repeat(PHP_EOL,$post_lines);

    // @note see promptd
    error_log("{$pre_lines}{$msg}{$post_lines}");
}


/**
 * Determine the apparent MIME type of a filename using the file extension!
 * 
 * If it's not recognized, the original input is returned.
 * 
 * @note Use finfo() etc for real content evaluation.
 */
function mime_by_name( $name ): string
{
    /**
     * Known content types by extension or common name, like "powerpoint".
     */
    static $types = [
        'aac'=>'audio/aac',
        'abw'=>'application/x-abiword',
        'arc'=>'application/x-freearc',
        'avif'=>'image/avif',
        'avi'=>'video/x-msvideo',
        'azw'=>'application/vnd.amazon.ebook',
        'bin'=>'application/octet-stream','binary'=>'application/octet-stream',
        'bmp'=>'image/bmp',
        'bz'=>'application/x-bzip',
        'bz2'=>'application/x-bzip2',
        'cda'=>'application/x-cdf',
        'csh'=>'application/x-csh',
        'css'=>'text/css',
        'csv'=>'text/csv',
        'doc'=>'application/msword',
        'docx'=>'application/vnd.openxmlformats-officedocument.wordprocessingml.document','word'=>'application/vnd.openxmlformats-officedocument.wordprocessingml.document',
        'eot'=>'application/vnd.ms-fontobject',
        'epub'=>'application/epub+zip',
        'gz'=>'application/gzip','gzip'=>'application/gzip',
        'gif'=>'image/gif',
        'htm'=>'text/html','html'=>'text/html',
        'ico'=>'image/vnd.microsoft.icon','favicon'=>'image/x-icon',
        'ics'=>'text/calendar',
        'jar'=>'application/java-archive',
        'jpeg'=>'image/jpeg','jpg'=>'image/jpeg',
        'js'=>'text/javascript','javascript'=>'text/javascript',
        'json'=>'application/json',
        'jsonld'=>'application/ld+json',
        'mid'=>'audio/midi','midi'=>'audio/midi',
        'mjs'=>'text/javascript',
        'mp3'=>'audio/mpeg',
        'mp4'=>'video/mp4',
        'mpeg'=>'video/mpeg','mpg'=>'video/mpeg',
        'mpkg'=>'application/vnd.apple.installer+xml',
        'odp'=>'application/vnd.oasis.opendocument.presentation',
        'ods'=>'application/vnd.oasis.opendocument.spreadsheet',
        'odt'=>'application/vnd.oasis.opendocument.text',
        'oga'=>'audio/ogg',
        'ogv'=>'video/ogg',
        'ogx'=>'application/ogg',
        'opus'=>'audio/opus',
        'otf'=>'font/otf',
        'png'=>'image/png',
        'pdf'=>'application/pdf',
        'php'=>'application/x-httpd-php',
        'ppt'=>'application/vnd.ms-powerpoint',
        'pptx'=>'application/vnd.openxmlformats-officedocument.presentationml.presentation','powerpoint'=>'application/vnd.openxmlformats-officedocument.presentationml.presentation',
        'rar'=>'application/vnd.rar',
        'rtf'=>'application/rtf',
        'sh'=>'application/x-sh',
        'svg'=>'image/svg+xml',
        'tar'=>'application/x-tar',
        'tif'=>'image/tiff','tiff'=>'image/tiff',
        'ts'=>'video/mp2t',
        'ttf'=>'font/ttf',
        'txt'=>'text/plain',
        'vsd'=>'application/vnd.visio',
        'wav'=>'audio/wav',
        'weba'=>'audio/webm',
        'webm'=>'video/webm',
        'webp'=>'image/webp',
        'woff'=>'font/woff',
        'woff2'=>'font/woff2',
        'xhtml'=>'application/xhtml+xml',
        'xls'=>'application/vnd.ms-excel',
        'xlsx'=>'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet','excel'=>'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet',
        'xml'=>'application/xml',
        'xul'=>'application/vnd.mozilla.xul+xml',
        'zip'=>'application/zip',
        '3gp'=>'video/3gpp',
        '3g2'=>'video/3gpp2',
        '7z'=>'application/x-7z-compressed',

        'ai'=>'application/illustrator',
        'eps'=>'application/postscript',
        'psd'=>'image/photoshop',
        'wma'=>'audio/x-ms-wma',
        'wmv'=>'video/x-ms-wmv',
        'xaml'=>'application/xaml+xml',
        'xap'=>'application/x-silverlight-app',
        'xps'=>'application/vnd.ms-xpsdocument',
        
        'atom'=>'application/atom+xml',
        'rss'=>'application/rss+xml',

        'vcard'=>'text/vcard',
        'form'=>'application/x-www-form-urlencoded'];

        return $types[strtolower(pathinfo('blahbuffer.'.$name,PATHINFO_EXTENSION))]??'';
}

/**
 * Recursively includes files of certain extensions into the PHP runtime.
 * 
 * They are included into the clean global scope.
 * 
 * @param string $path The directory to include files from - WHICH COULD BE /etc !!
 * 
 * @note Generally used when caching/production.
 * @important This is not a security mechanism; pay attention.
 * @todo need to figure fit with opcache
 */
function include_dir( string $path,string $path_prefix = '',array $include_exts = [] ): void
{
    if( empty($include_exts) )
        $include_exts = ['html','php','inc','tpl','htm','css','js'];

    if( !empty($path_prefix) )
        $abs_path = $path_prefix.DIRECTORY_SEPARATOR.$path;
    else
        $abs_path = APP_ROOT.DIRECTORY_SEPARATOR.$path;

    if( ($abs_path = realpath($abs_path)) === FALSE )
        throw new e500("Directory '$abs_path' ($path) ($path_prefix) not valid.");

    try
    {
        $file_i = new \RecursiveIteratorIterator(new \RecursiveDirectoryIterator($abs_path,\FilesystemIterator::KEY_AS_PATHNAME & \FilesystemIterator::SKIP_DOTS & \RecursiveIteratorIterator::CATCH_GET_CHILD));

        foreach( $file_i as $pathname => $file_j )
        {
            if( $file_j->isFile() )
            {
                $ext = strtolower(pathinfo($pathname,PATHINFO_EXTENSION));
                if( !in_array($ext,$include_exts) )
                    continue;

                include($pathname);

                // @todo add debugging
                //_stdo("{$char_cnt}include $pathname;
            }
        }
    }
    catch( \Exception $e )
    {
        throw new e500($e->getMessage(). "(permission denied/not found for $pathname)");
    }
}

