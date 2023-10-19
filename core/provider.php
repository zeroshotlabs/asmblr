<?php declare(strict_types=1);
/**
 * @file provider.php Conherent interface for accessing external data, currently from the filesystem.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;

use asm\_e\e404,asm\_e\e500;
use function asm\sys\mime_by_name;

/**
 * For some future use maybe.
 */
interface provider { }

/**
 * Tools for working with files and directories:
 *   - Recursively loads files recursively from a directory and store them 
 *     into an URL keyed array, providing read access by URL.  The URL may
 *     optionally be rewritten (set_reroot()).
 *   - Retrieve a single file on request from a root directory,
 *     optionally rewrote (use_root())
 * 
 * Files and directories read from disk relative to self::$fs_root, which
 * defaults to APP_ROOT.
 * 
 * Blatant '..' bad behavior should be caught BUT THIS IS NOT A SECURITY MECHANISM!
 * 
 * Files loaded by load_dir() accessible by URL, that is, the path of the file
 * rooted to the loaded or reroot'd path.
 * 
 * For example, a directory of files named such as "foo/bar/baz.txt" will be
 * loaded as an associative array with a keys such as "foo/bar/baz.txt".
 * 
 * Rerooting will make the files available at different URLs (SSGs).
 * 
 * The file's content type is determined by it's extension.
 * 
 * No encoding or other modification is made to the path and file names.
 * Paths aren't checked, validated or realpath()'d.
 * 
 * @note This isn't a security mechanism - it's dangerous!
 * @note Double slashes will read anything from the drive's root!
 * 
 * @note This is basically for Unix, though easy changes for Windows.
 * 
 * @note Pay attention to trailing slashes when renaming/rerooting.
 * @note It's probably not a good idea to use both load_dir() and
 *       individual files - use a new object.
 * @todo maybe the 404s should be replaced by returns and the caller handles.
 */
class filesystem implements provider
{
    public $fs_root = APP_ROOT;
    public $include_exts = ['html','txt','php','tpl','htm','css','js'];

    public $files = [];

    /**
     * Store reroot mapping.
     * 
     * @note This is used differently for load_dir() - DO NOT MIX USAGE WITH FILE GETS.
     */
    public $reroot = [];

    // two slashes
    /**
     * Instantiate a new filesystem object, optionally rooted at a path, to
     * read files from.
     * 
     * If $path isn't empty, the object will be rooted and rerooted, as specified.
     * 
     * If $path has two leading forward slashes, it is taken as the absolute path.
     * Otherwise, $fs_root will be prefixed which defaults to APP_ROOT.
     * 
     * If $reroot is specified, it will used to rewrite the URL upon file fetch.
     * 
     * @param string $root_path The path to read files from.
     * @param string|array $reroot The from => to remap or remove a URL prefix URL -> new path
     * @param array $include_exts The file extensions to include (only used in load_dir()).
     * 
     * @note Rerooting is done using str_replace - pay attention to the slashes.
     * @note Construct without values to use the object with load_dir().
     */
    public function __construct( string $root_path = '',string|array $reroot = [],$include_exts = [] )
    {
        if( !empty($root_path) )
        {
            if( strpos($root_path,'//') === 0 )
                $this->fs_root = substr($root_path,1);
            else
                $this->fs_root = $this->fs_root.DIRECTORY_SEPARATOR.$root_path;

            if( !empty($reroot) )
                $this->reroot = is_string($reroot)?['/',$reroot]:$reroot;
            
            if( !empty($include_exts))
                $this->include_exts = $include_exts;
        }
    }


    /**
     * Returns a file by URL, which can be reroot'd (aliased) when the object is created.
     * 
     * With a $root_path of /var/www/html and no reroot:
     *  - requests for /index.html would fetch /var/www/html/index.html
     *  - requests for /admin/home/index.html would fetch /var/www/html/admin/home/index.html
     * 
     * With a reroot of ['/prefix/','/']:
     *  - requests for /prefix/index.html would fetch /var/www/html/index.html
     * 
     * With a reroot of ['/','/prefix/']:
     *  - requests for /index.html would fetch /var/www/html/prefix/index.html
     * 
     * @param string $url The URL path to load, relative to the established root/reroot.
     * 
     * @note Rerooting is done using str_replace - pay attention to the slashes.
     * @note This will fetch any file, regardless of extension or location - like /etc/passwd
     * @note Do not mix this with load_dir() in the same object.
     * @note This is not a security mechanism!
     * @todo caching?
     * @todo NO REROOTNG - DO WE NEED IT HERE?!?!?
     */    
    public function __get( string $url ): array|null
    {
        throw new e500("Is this used?  $url");
        
        // "editor.js/node_modules/es-to-primitive/test/es6.js
        if( strpos($url,'/../') !== FALSE )
            throw new e404("Suspicious path: $url");

        // if( !empty($this->reroot) )
        //     $true_url = $this->fs_root.DIRECTORY_SEPARATOR.str_replace($this->reroot[0],$this->reroot[1],$url);
        // else
        //     $true_url = $url;


//        $true_url = $this->fs_root.DIRECTORY_SEPARATOR.str_replace($this->reroot[0],$this->reroot[1],$url);
        $true_path = realpath($this->fs_root.DIRECTORY_SEPARATOR.$url);
//        var_dump($this->fs_root.DIRECTORY_SEPARATOR.$url);
        if( !$true_path || is_file($true_path) === FALSE )
            throw new e404(($this->fs_root.DIRECTORY_SEPARATOR.$url)." ($url) not found");

        // _stde("\n{$char_cnt}
        $file = ['content_type'=>mime_by_name($url)??mime_by_name('asdfas.html'),
                         'path'=>$true_path,
                      'content'=>file_get_contents($true_path)];
        $file['content_length'] = strlen($file['content']);

        return $file;
    }

    //     $url = substr($full_path,strpos($full_path,$path));

    //     if( !empty($reroot[0]) && !empty($reroot[1]) )
    //         $url = str_replace($reroot[0],$reroot[1],$path);
    //     else if( !empty($reroot[0]) )
    //         $url = substr($path,strlen($reroot[0]));
    //     else
    //         $url = $path;

    /**
     * Given a root path, the path of a file, and a prefix, replace or
     * remove the prefix from the path.
     * 
     * This is for filesystems, not URLs:
     *   filesystem path -> new root URL
     * 
     * @note Used by load_dir().
     */
    public function reroot_path( $rootpath,$filepath,$prefix,$reroot )
    {
        $url = substr($filepath,strpos($rootpath,$prefix));

        if( !empty($reroot[0]) && !empty($reroot[1]) )
            $url = str_replace($reroot[0],$reroot[1],$url);
        else if( !empty($reroot[0]) )
            $url = substr($url,strpos($url,$reroot[0])+strlen($reroot[0]));

        return $url;
    }


    /**
     * Recursively loads files of certain extensions into an URL keyed array,
     * overwriting any conflicting entry in self::$dirs.
     * 
     * If $load_content is false, only the array structure is created in $files,
     * otherwise the file contents is also loaded.
     * 
     * @param string $path The file path to load which will become the 'URL'.
     * @param string $reroot The prefix of the path to remove, and optionally replace, relocating the root.
     * @param bool $load_content True to load full file content.
     * 
     * @note The URL is formed as the path and file name rooted at the specified directory.
     * @note $path isn't checked, validated or realpath()'d and it's just a string replace.
     * @note Generally used when caching/production.
     * @important This is not a security mechanism though a basic .. check is performed.
     * 
     * @todo test - need realpath?
     */
    public function load_dir( string $path,string|array $reroot = [],bool $load_content = false ): void
    {
        if( strpos($path,'/../') !== FALSE )
            throw new e500("Suspicious path: $path");

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
                    
                    $url = $this->reroot_path($full_path,$pathname,$path,$reroot);

                    $this->files[$url] = ['content_type'=>mime_by_name($ext),
                                                 'path'=>$pathname,
                                              'content'=>$load_content?file_get_contents($pathname):''];
                    $file_cnt++;
                    $char_cnt += strlen($this->files[$url]['content']);
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
