<?php declare(strict_types=1);
/**
 * @file app.php Application runtime.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;
use asm\types\url;
use asm\sys;
use asm\http;
use asm\cli;
use asm\_e\e500;


/**
 * The primary application runtime.
 * 
 * This centrally provides access to the runtime environment, comprised primarily
 * of routing, execution/endpoint, and template/out.
 * 
 * This also holds configuration parameters, and provides access to the request,
 * and is the default context for endpoint execution.
 * 
 * @todo add linker for endpoints
 */ 
abstract class app
{
//    use linker;

    public readonly bool $IsProduction;
    public readonly bool $IsCLI;


    protected $HasSuperRoot = TRUE;

    protected $route_table = [];


    /**
     * @todo add config overrides Key/value array of custom application config variables, that override config.
     */
    public function __construct( protected \asm\config $config,protected \asm\request $request )
    {
        $this->IsProduction = $this->config->IsProduction;
        $this->IsCLI = $this->request->IsCLI;
    }

    /**
     * Builds the route_table (execution queue) using explicit matching:
     *  - loop over the requested URL path, from root down
     *  - each increasing URL is matched as a directory with trailing slash (IsDir = true)
     *  - finally a leaf (IsDir = false) is matched
     * 
     * This means that the trailing slash is important and URLs are matched explicitly.
     * For example:
     *  - request: /admin
     *    matches: /admin but not /admin/
     *  - request: /admin/users
     *    matches: /admin/ and /admin/users but not /admin nor /admin/users/
     *  - request: /admin/users/username
     *    matches: /admin/ and /admin/users/ but not /admin nor /admin/users
     *  
     * The super-root (//) is always executed first.
     * 
     * Similarly, double trailing slashes of an endpoint will make it execute
     * for directory and non-directory (leaf) requests, i.e.
     *  - request: /admin/anything or /admin or /admin/
     *    matches: /admin// will match all
     * 
     * These are coined "greedy" roots.
     * 
     * @return array &$route_table Reference to the endpoints execution queue, in order.
     * @todo Abstract this out as it's own closure/class for custom routers.
     * @note This is for ALL web requests - including the FrontStack.
     * @note Be minful when using greedy roots on high endpoints.
     */
    public function &route(): array
    {
        $this->route_table = [];
        $this->route_table[] = $this->config->endpoints_url_map['//'];

        foreach( $this->request->route_path->ordered() as $path )
        {
            if( isset($this->config->endpoints_url_map[$path.'//']) )
                $this->route_table[] = $this->config->endpoints_url_map[$path.'//'];

            if( isset($this->config->endpoints_url_map[$path])  )
                $this->route_table[] = $this->config->endpoints_url_map[$path];
            else if( isset($this->config->endpoints_url_map[$path.'/']) )
                $this->route_table[] = $this->config->endpoints_url_map[$path.'/'];
        }

        return $this->route_table;
    }


    /**
     * Execute an endpoint.
     * 
     * This is used to execute a single endpoint.  If there is a period in the 'exec'
     * element, it's split.
     *  - if there are two elements, the first is taken as the class to instantiate (__construct() is thus called)
     *    and the second as the method to call. 
     *  - if there is only a single element, it is taken as the class to instantiate (__construct() is called)
     *    and execute (__invoke() is called).
     * 
     * @note app classes are assumed in the global namespace of the app (see $e below).
     */
    public function exec_endpoint( array|string $endpoint ): mixed
    {
        if( is_string($endpoint) )
        {
            if( isset($this->config->endpoints[$endpoint]) )
                $endpoint = $this->config->endpoints[$endpoint];
            else
                throw new e500("Endpoint '$endpoint' not found.");
        }

        $exec = explode('.',$endpoint['exec']??'');

        // @note Execute a routing or CLI endpoint which is instantiated and __invoked()'d.
        //       The executing object persists in $this->route_table[$exec[0]]['obj'] and will be
        //       reused if nessecary.
        // @todo check namespaces.  this doesn't actually care about interface implementation.
        if( count($exec) == 1 )
        {
            if( !isset($this->route_table[$exec[0]]['obj']) )
            {
                $e = "\\$exec[0]";                
                $exec_obj = $this->route_table[$exec[0]]['obj'] = new $e($this);
            }
            else
                $exec_obj = $this->route_table[$exec[0]]['obj'];

            return $exec_obj($this->request,$endpoint);
        }
        // @note Execute a leaf endpoint.
        else if( count($exec) == 2 )
        {
            if( !isset($this->route_table[$exec[0]]['obj']) )
            {
                $e = "\\$exec[0]";
                $exec_obj = $this->route_table[$exec[0]]['obj'] = new $e($this);
            }
            else
                $exec_obj = $this->route_table[$exec[0]]['obj'];

            return $exec_obj->{$exec[1]}($this->request,$endpoint);
        }
        else
            throw new e500("Malformed endpoint exec '{$endpoint['exec']}");
    }
}




// @todo app/config /etc  caching
    // /**
    //  * @var string $CacheDir
    //  * Absolute path to the asmblr local cache.
    //  *
    //  * @note This must be set explicitly in the extending class.
    //  *
    //  * @todo For future GAE integration:
    //  *      @c gs://asmblr-mc-tmp/
    //  *      @c array('gs'=>array('Content-Type'=>'text/plain'))
    //  */
    // public $CacheDir = '';
    // /**
    //  * @var boolean $CacheManifest
    //  * TRUE to cache the manifest to the local disk.
    //  * 
    //  * @note This is configured in the application array in @c index.php
    //  */
    // public $CacheManifest = FALSE;

    // /**
    //  * @var boolean $CacheApp
    //  * TRUE to cache the app into a local app-file.
    //  *
    //  * @note This is configured in the application array in @c index.php
    //  */
    // public $CacheApp = FALSE;


        /**
     * Default user error handler.
     *
     * Handle an application or PHP error.  This method is set using
     * set_error_handler() in App::__construct().
     *
     * @param int $errno The message severity as a PHP constant.
     * @param string $errstr The error message.
     * @param string $errfile The filename from which the message came.
     * @param int $errline The line number of the file.
     * @param array $errcontext Local scope where the error occurred.
     * @retval boolean FALSE if the error should be ignored, TRUE if not.
     *
     * @note errcontext can be huge.
     */
    // public function ErrorHandler( $errno,$errstr,$errfile,$errline,$errcontext = NULL )
    // {
    //     // error surpressed with @
    //     if( error_reporting() === 0 )
    //         return FALSE;

    //     if( in_array($errno,array(E_WARNING,E_USER_WARNING)) )
    //         $errno = 'WARN';
    //     else if( in_array($errno,array(E_NOTICE,E_USER_NOTICE)) )
    //         $errno = 'INFO';
    //     else
    //         $errno = 'ERROR';

    //     $BT = array_merge(array("[{$errfile}:{$errline}]"),Debug::Backtrace());

    //     Log::Log($errstr,$errno,$BT,$errcontext);

    //     return TRUE;
    // }



//////////////////

//     /**
//      * Compile an application into a single executeable string.
//      *
//      * This loads functions, templates and PHP code from the filesystem into memory as a single string.
//      *
//      * This will prefix Template names with their parent directory's name if more than one-level deep.  This
//      * also parses templates for the \@\@\@ notation and is partially duplicated in TemplateSet::LoadFile
//      *
//      * If @c $Dirs is provided, it must be of the form:
//      *   - @c array('lib'=>array(),'functions'=>array(),'templates'=>array())
//      *
//      * where each element is an array of directories to load from.
//      *
//      * @param array $Dirs An array of directories to load the application from.
//      * @throws Exception Unreadable AppRoot
//      * @retval string The compiled application.
//      *
//      * @note If App::$CacheApp is FALSE, the templates remain on disk and will be loaded by
//      *       TemplateSet::LoadFile upon rendering.  This helps with debugging since PHP's error messages
//      *       will reference a real file/line.
//      * @note Only files with the following extensions are loaded: <tt> .inc .php .tpl .html .js .css </tt>
//      * @note This doesn't include vendor or themes.
//      */
//     protected function BuildApp( $Dirs = array() )
//     {
//         if( empty($Dirs) )
//         {
//             $AppRoot = $this->Manifest['AppRoot'];

//             if( !is_dir($AppRoot) || !is_readable($AppRoot) )
//                 throw new Exception("Unreadable AppRoot '{$AppRoot}'");

//             $Dirs = array('lib'=>array("{$AppRoot}lib"),
//                           'functions'=>array("{$AppRoot}functions"),
//                           'templates'=>array("{$AppRoot}templates"));
//         }

//         $Hostname = $this->Hostname;

//         $AppFile = '';

//         // PHP Bug - can't use FilesystemIterator::CURRENT_AS_PATHNAME |  FilesystemIterator::KEY_AS_FILENAME with recursive dirs
//         $Flags = \FilesystemIterator::SKIP_DOTS | \FilesystemIterator::UNIX_PATHS;

//         // subdirectories are NOT prefixed
//         foreach( $Dirs['lib'] as $D )
//         {
//             $dir = new \RecursiveDirectoryIterator($D,$Flags);
//             $fs = new \RecursiveIteratorIterator($dir);
//             foreach( $fs as $K => $V )
//             {
//                 $PI = pathinfo($K);
//                 if( in_array(strtolower($PI['extension']),array('php','inc')) === FALSE )
//                     continue;

//                 // if we're not caching, just include the file
//                 // this helps tremendously with useful error messages - ditto below as well
//                 if( empty($this->CacheApp) )
//                 {
//                     include $K;
//                 }
//                 else
//                 {
//                     $AppFile .= "\n\n\n/*** {$K} ***/";

//                     $T = php_strip_whitespace($K);
//                     if( strpos($T,'<?php') === 0 )
//                         $AppFile .= "\n".substr($T,5);
//                     else
//                         $AppFile .= $T;
//                 }
//             }
//         }

//         // subdirectories are NOT prefixed
//         foreach( $Dirs['functions'] as $D )
//         {
//             $dir = new \RecursiveDirectoryIterator($D,$Flags);
//             $fs = new \RecursiveIteratorIterator($dir);
//             foreach( $fs as $K => $K )
//             {
//                 $PI = pathinfo($K);
//                 if( in_array(strtolower($PI['extension']),array('php','inc')) === FALSE )
//                     continue;

//                 if( empty($this->CacheApp) )
//                 {
//                     include $K;
//                 }
//                 else
//                 {
//                     $AppFile .= "\n\n\n/*** {$K} ***/";

//                     $T = php_strip_whitespace($K);
//                     if( strpos($T,'<?php') === 0 )
//                         $AppFile .= "\n".substr($T,5);
//                     else
//                         $AppFile .= $T;
//                 }
//             }
//         }

//         // @todo quick hack though this approach should probably be applied to pageset, and app for the lib
//         // this has also resulted in a couple of other hacks, though maybe good
//         $Templates = $this->html->LoadDir($Dirs['templates'][0],$this,TRUE);

        
//         return "<?php\n\n{$AppFile} \n\n \$this->Templates = ".var_export($Templates,TRUE).';';

//         // // @todo @see TemplateSet::LoadDir
//         // throw new Exception("BuildApp TODO - templates");
// /* 
//         // subdirectories ARE prefixed and merge any Function definition from the manifest
//         // see also TemplateSet::Load()
//         $Templates = array();
//         foreach( $Dirs['templates'] as $D )
//         {
//             $AppFile .= "\n\n\n / *** {$D} *** /";
//             $dir = new \RecursiveDirectoryIterator($D,$Flags);
//             $fs = new \RecursiveIteratorIterator($dir);
//             foreach( $fs as $K => $V )
//             {
//                 $PI = pathinfo($K);
//                 if( in_array(strtolower($PI['extension']),array('php','inc','tpl','html','css','js')) === FALSE )
//                     continue;

//                 $P = Path::Init($K);
//                 $P = Path::Bottom($P,2);

//                 if( $P['Segments'][0] !== 'templates' )
//                     $Prefix = $P['Segments'][0].'_';
//                 else
//                     $Prefix = '';

//                 $Buf = file_get_contents($K);

//                 if( strpos(substr($Buf,0),"\n@@@") === FALSE )
//                 {
//                     $P['Segments'][1] = pathinfo($P['Segments'][1],PATHINFO_FILENAME);

//                     if( !empty($this->Manifest['Templates'][$Prefix.$P['Segments'][1]]) )
//                         $F = $this->Manifest['Templates'][$Prefix.$P['Segments'][1]];
//                     else
//                         $F = array();

//                     // if we're not caching, don't store the body - it'll be include()'d in TemplateSet::__call()
//                     // if we are, prepend the start tag so that it can be eval()'d without string munging
//                     // same for below
//                     if( empty($this->CacheApp) )
//                         $Buf = '';
//                     else
//                         $Buf = "{$Buf}";

//                     $Templates[$Prefix.$P['Segments'][1]] = Template::Init($Prefix.$P['Segments'][1],$K,$F,$Buf);
//                 }
//                 // we'd like to retire this (or at least only have in TemplateSet::LoadFile)
//                 //  but MC currently depends on it
//                 else
//                 {
//                     // this regex needs to be more robust, including handling empty frags, frags that don't
//                     // start with whitespace, those that end with a comment, etc. - these are always set in the body
//                     // and never have a path
//                     // the naming scheme here is DirName_Filename or just Filename
//                     $B = preg_split("/\s*@@@(\w+[a-zA-Z0-9\-]*)/m",$Buf,0,PREG_SPLIT_DELIM_CAPTURE|PREG_SPLIT_NO_EMPTY);
//                     $CNT = count($B);
//                     for( $i = 0; $i < $CNT; $i+=2 )
//                     {
//                         if( !empty($this->Manifest['Templates'][$Prefix.$B[$i]]) )
//                             $F = $this->Manifest['Templates'][$Prefix.$B[$i]];
//                         else
//                             $F = array();

//                         $Templates[$Prefix.$B[$i]] = Template::Init($Prefix.$B[$i],'',$F," {$B[$i+1]}");
//                     }
//                 }
//             }
//         } 



