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
 * @note Use finfo() etc for real content evaluation.
 */
function mime_by_name( $name ): string|null
{
    /**
     * Known content types by extension or common name, like "powerpoint".
     */
    static $types = [
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

        'form'=>'application/x-www-form-urlencoded'];

        return $types[strtolower(pathinfo('blah.'.$name,PATHINFO_EXTENSION))]??null;
}

/**
 * Recursively loads files of single files or recursively of directories 
 * into an URL keyed array.
 * 
 * Files and directories read from desk relative to self::$fs_root, which
 * defaults to APP_ROOT.
 * 
 * While this loads directories & files recursively, it flattens the structure
 * out to a string keyed associative array of files where each key is the "URL";
 * that is, the path of the file rooted to the loaded or $reroot'd path.
 * 
 * For example, a directory of files named such as "foo/bar/baz.txt" will be
 * loaded as an associative array with a keys such as "foobar/baz.txt".
 * 
 * Rerooting load_file() will make the file available at a different URL,
 * overriding any any existing endpoint with the same URL.
 * 
 * Rerooting with load_dir() allows for alteration of the file's accessed-by
 * an alternate base URL.
 * 
 * No encoding or other modification is made to the path and file names.
 * Paths aren't checked, validated or realpath()'d.
 * @note This isn't a security mechanism.
 * 
 * @note Pay attention to trailing slashes when renaming/rerooting.
 * 
 * @todo need also for including PHP code/etc; need to figure fit with opcache
 */
class fs_load
{
    public $fs_root = APP_ROOT;
    public $include_exts = ['html','txt','php','tpl','htm','css','js'];

    public $files = [];
    public $dirs = [];


    /**
     * Convenience method to access the loaded files' contents.
     * 
     * The $url is first tried as a single file, then as part of a loaded directory.
     * 
     * @todo caching/reading from disk.
     */
    public function __get( string $url ): string|null
    {
        return $this->files[$url]['content']??$this->dirs[$url]['content']??null;
    }

    /**
     * Loads a file, keyed by it's URL, overwriting any conflicting entry in self::$files.
     * 
     * @param string $path The file path to load which will become the 'URL'.
     * @param string $rename The prefix of the path to remove, and optionally replace, relocating the root.
     * @param bool $load_content True to load full file content.
     * @throws e500 If the file isn't readable.
     * 
     * @note The URL is formed as the path and file name rooted at the specified directory.
     */
    function load_file( $path,string|array $reroot = [],$load_content = true )
    {
        $full_path = $this->fs_root.$path;

        if( is_string($reroot) )
            $reroot = [$reroot,];

        if( !is_file($full_path) )
            throw new e500("File '$full_path' not readable.");

        $url = substr($full_path,strpos($full_path,$path));

        if( !empty($reroot[0]) && !empty($reroot[1]) )
            $url = str_replace($reroot[0],$reroot[1],$path);
        else if( !empty($reroot[0]) )
            $url = substr($path,strlen($reroot[0]));
        else
            $url = $path;

        $this->files[$url] = ['content_type'=>mime_by_name($path),
                                      'path'=>$full_path,
                                   'content'=>$load_content?file_get_contents($full_path):''];

        // @todo add debugging
        // _stde("\n{$char_cnt} - $path");
        // $char_cnt = strlen($this->file[$url]['content']);
    }

    function include_dir( string $path ): void
    {
        $full_path = $this->fs_root.$path;        

        if( !is_dir($full_path) )
            throw new e500("Directory '$full_path' not valid.");

        try
        {
            $file_i = new \RecursiveIteratorIterator(new \RecursiveDirectoryIterator($full_path,\FilesystemIterator::KEY_AS_PATHNAME & \FilesystemIterator::SKIP_DOTS & \RecursiveIteratorIterator::CATCH_GET_CHILD));

            foreach( $file_i as $pathname => $file_j )
            {
                if( $file_j->isFile() )
                {
                    $ext = strtolower(pathinfo($pathname,PATHINFO_EXTENSION));
                    if( !empty($this->include_exts) && !in_array($ext,$this->include_exts) )
                        continue;

                    include $pathname;
                }
            }
        }
        catch( \Exception $e )
        {
            throw new e500($e->getMessage(). "(permission denied/not found for $pathname)");
        }

    }

    /**
     * Recursively loads files of certain extensions into an URL keyed array,
     * overwriting any conflicting entry in self::$dirs.
     * 
     * If $load_content is false, only the array structure is created, otherwise
     * contents of each file is also loaded.
     * 
     * @param string $path The file path to load which will become the 'URL'.
     * @param string $reroot The prefix of the path to remove, and optionally replace, relocating the root.
     * @param bool $load_content True to load full file content.
     * 
     * @note The URL is formed as the path and file name rooted at the specified directory.
     * @note $path isn't checked, validated or realpath()'d and it's just a string replace.
     * @important This is not a security mechanism.
     */
    function load_dir( string $path,string|array $reroot = [],bool $load_content = false ): void
    {
        $full_path = $this->fs_root.$path;

        if( !is_dir($full_path) )
            throw new e500("Directory '$full_path' not valid.");

        if( is_string($reroot) )
            $reroot = [$reroot,];

        try
        {
            $file_i = new \RecursiveIteratorIterator(new \RecursiveDirectoryIterator($full_path,\FilesystemIterator::KEY_AS_PATHNAME & \FilesystemIterator::SKIP_DOTS & \RecursiveIteratorIterator::CATCH_GET_CHILD));
            $file_cnt = $char_cnt = 0;

            foreach( $file_i as $pathname => $file_j )
            {
                if( $file_j->isFile() )
                {
                    $ext = strtolower(pathinfo($pathname,PATHINFO_EXTENSION));
                    if( !empty($this->include_exts) && !in_array($ext,$this->include_exts) )
                        continue;

                    $url = substr($pathname,strpos($full_path,$path));
                    
                    if( !empty($reroot[0]) && !empty($reroot[1]) )
                        $url = str_replace($reroot[0],$reroot[1],$url);
                    else if( !empty($reroot[0]) )
                        $url = substr($url,strpos($url,$reroot[0])+strlen($reroot[0]));

                    $this->dirs[$url] = ['content_type'=>mime_by_name($ext),
                                          'path'=>$pathname,
                                          'content'=>$load_content?file_get_contents($pathname):''];
                    $file_cnt++;
                    $char_cnt += strlen($this->dirs[$url]['content']);
                    // @todo add debugging
                    //_stdo("{$char_cnt} - $pathname - $url");
                }
            }
        }
        catch( \Exception $e )
        {
            throw new e500($e->getMessage(). "(permission denied/not found for $pathname)");
        }

        // @todo add debugging
        // return [$file_cnt,$char_cnt];
    }
}

