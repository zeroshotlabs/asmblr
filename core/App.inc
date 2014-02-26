<?php
/**
 * @file App.inc asmblr application controller.
 * @author Stackware, LLC
 * @version 4.0
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * Base configuration, controller and runtime container for an application.
 *
 * It can be extended and customized in an application's Load.inc.
 *
 * @note This is your GOD anti-pattern, including dynamic properties.  The
 *       mess goes here, so the rest is clean.
 */
abstract class App
{
    // gs://asmblr-mc-tmp/
    // array('gs'=>array('Content-Type'=>'text/plain'))
    // TODO: may need a cnvyr specific cache dir
    public $CacheDir = '';

    public $Hostname;
    public $AppRoot;

    public $CacheManifest;
    public $CacheApp;

    public $RoutingPS;

    public $OrderedMatch;
    public $ExactMatch;

    public $ClosestMatchName;

    public $Request;
    public $Manifest;

    public $Pages;
    public $PageMaps;
    public $Config;
    public $Templates;


    /**
     * Default app build/execute.
     *
     * @todo Implement locking/flock() for manifest/app cache file creation.
     */
    public function __construct( $App,$Request )
    {
        $this->Hostname = $App['Hostname'];
        $this->AppRoot = $App['AppRoot'];
        $this->CacheManifest = $App['CacheManifest'];
        $this->CacheApp = $App['CacheApp'];
        $this->RoutingPS = $App['RoutingPS'];

        $this->Request = $Request;

        // seems to hold us for linux/windows/gae - though the str_replace is annoying for windows
        if( empty($this->CacheDir) )
            $this->CacheDir = str_replace('\\','/',sys_get_temp_dir());

        // make this object global :)
        $GLOBALS['asmapp'] = $this;


        // build/cache our manifest
        if( $this->CacheManifest === TRUE )
        {
            if( (@include "{$this->CacheDir}/{$this->Hostname}.manifest.inc") === FALSE )
            {
                $this->Manifest = $this->BuildManifest($App['ManifestURL']);
                file_put_contents("{$this->CacheDir}/{$this->Hostname}.manifest.inc",'<?php $this->Manifest = '.var_export($this->Manifest,TRUE).';');
            }
        }
        else
        {
            // clear an existing cache
            @unlink("{$this->CacheDir}/{$this->Hostname}.manifest.inc");
            $this->Manifest = $this->BuildManifest($App['ManifestURL']);
        }

        // set local properties for the manifest - would like to streamline as it's wasteful
        // but we'd need to refactor how we pull from the cache
        $this->Config = $this->Manifest['Config'];
        $this->Pages = $this->Manifest['Pages'];
        $this->PageMaps = $this->Manifest['PageMaps'];

        // build/cache our app file
        // include/eval of this will set our $this->Templates property
        if( $this->CacheApp === TRUE )
        {
            if( (@include "{$this->CacheDir}/{$this->Hostname}.app.inc") === FALSE )
            {
                file_put_contents("{$this->CacheDir}/{$this->Hostname}.app.inc",$this->BuildFile());
                include "{$this->CacheDir}/{$this->Hostname}.app.inc";
            }
        }
        else
        {
            // clear an existing cache
            @unlink("{$this->CacheDir}/{$this->Hostname}.app.inc");
            $T = $this->BuildFile();
            eval('?>'.$T);
        }


        // now proceed by applying general purpose config settings

        if( empty($this->Config['Status']) || $this->Config['Status'] !== 'Active' )
            HTTP::_400();

        // set a base directory for now this isn't supported (problematic on Windows)
//        if( !empty($this->Config['open_basedir']) )
//            $this->OpenBaseDir($this->Config['open_basedir']);

        // Activate the Framewire error handler
        if( empty($this->Config['error_handler']) )
            set_error_handler(array($this,'ErrorHandler'));
        else
            set_error_handler(array($this,$this->Config['error_handler']));


        // Calculate app-wide URLs, modifying our Request array.  This creates
        // $SiteURL and $MatchPath elements which are used for creating URLs (LinkSet) and
        // matching pages (PageSet), respectively.
        Request::CalcURLs($this->Request,$this->Config['BaseURL']);


        // instantiate system objects
        // this sets dynamic properties - know your names

        // one or more page sets are supported though there is always a single routing page set
        // as defined by $this->RoutingPS
        // the routing page set and it's linker are always available as ps and lp, respectively
        // other pagesets/linkers can be instantiated manually if needed
        if( empty($this->RoutingPS) || empty($this->Pages[$this->RoutingPS]) )
            throw new Exception("Invalid routing page set for '{$this->Hostname}'");

        $this->ps = new PageSet($this->Pages[$this->RoutingPS],$this->PageMaps[$this->RoutingPS]);
        $this->lp = new LinkPage($this->ps,$this,$this->Request['SiteURL']);

        // links for cnvyr managed resources (images, css, js, fonts/etc)
        $this->lc = new Linkcnvyr($this->ps,$this,$this->Request['SiteURL']);

        // TODO: needs to be configuration aware
        $this->html = new enUSHTMLSet($this);

        // general purpose key/value store for in-template info (meta-tags, CSS classes, etc)
        $this->page = new KeyValueSet;

        // session based UI messages/alerts
        $this->msg = new Messager;

        // track form/request validation using a default 'has-error' CSS class
        $this->vr = new ValidationReport('has-error');

        // templates need access to some of these Connect() them in
        // if you later Connect() something using the same name it will overwrite the object here
        $this->html->Connect(array('lp'=>$this->lp,'lc'=>$this->lc,'page'=>$this->page,'msg'=>$this->msg,'vr'=>$this->vr));


        // reference code for loading/connecting to mongo
        /*
        \asm\Inc::Ext('Mongo.inc');
        $mongo = new \asm\Mongo;
        $this->mydb = $mongo->Alias('mydb','mydb');
        */
    }

    /**
     * Match pages and execute the application.
     */
    public function Execute()
    {
        // match pages against the MatchPath to determine our executing page
        $this->OrderedMatch = $this->ExactMatch = NULL;

        // first attempt a hierarchal ordered match/execute - most general to most specific URL path
        if( $this->Request['MatchPath']['IsRoot'] === FALSE )
        {
            foreach( \asm\Path::Order($this->Request['MatchPath']) as $V )
            {
                if( ($this->OrderedMatch = $this->ps->Match($V)) !== NULL )
                {
                    $this->ClosestMatchName = $this->OrderedMatch['Name'];
                    break;
                }
            }
        }

        // if an ordered match isn't found, attempt an exact match.  if the ordered match is
        // Weak, an exact match is also allowed
        // i.e.  default:  /admin/ matches then a page with /admin/something will NOT match
        //          Weak:  /admin/ matches then a page with /admin/something WILL match
        if( $this->OrderedMatch === NULL || (strpos($this->OrderedMatch['Status'],'Weak') !== FALSE) )
        {
            $this->ExactMatch = $this->ps->Match(\asm\Path::ToString($this->Request['MatchPath']));
            $this->ClosestMatchName = $this->ExactMatch['Name'];
        }

        // if no pages, it's a 404
        if( empty($this->OrderedMatch) && empty($this->ExactMatch) )
            $this->NoPageHandler();

        // or if one isn't active
        if( (!empty($this->OrderedMatch['Status']) && (strpos($this->OrderedMatch['Status'],'Active') === FALSE))
        || (!empty($this->ExactMatch['Status']) && ($this->ExactMatch['Status'] !== 'Active')) )
            $this->NoPageHandler();


        // apply additional config parameters based on our environment/page

        // asmblr supports CLI execution natively, though some config directives
        // only make sense when serving an HTTP request, or for certain pages
        if( $this->Request['IsCLI'] === FALSE )
        {
            // IsBaseHostname indicates whether the request is using the full hostname
            // configured by BaseURL.  If not, redirect to it.
            if( $this->Config['ForceBaseHostname'] === TRUE && $this->Request['IsBaseHostname'] === FALSE )
            {
                $this->Request['Hostname'] = $this->Request['BaseURL']['Hostname'];
                HTTP::Location($this->Request);
            }

            // start a session if appropriate
            // TODO: needs review/check
            if( !empty($this->Config['StartSession']) && (strpos($this->Config['PageNoSession'],$this->OrderedMatch['Name']) === FALSE) && (strpos($this->Config['PageNoSession'],$this->ExactMatch['Name']) === FALSE))
                session_start();

            // note that this can lead to decoding errors in a browser if the request doesn't complete,
            // this includes var_dump()/exit debugging
            // TODO: perhaps this should be moved/removed
            if( !empty($this->Config['zlib_output']) )
                ini_set('zlib.output_compression',TRUE);

            // TODO: do we need this?  Does it do anything?  needs testing.
            // do we need mb_output_handler()?
            if( !empty($this->Config['mb_http_output']) )
                mb_http_output($this->Config['mb_http_output']);

            // these can also be handled when creating the enUSHTMLSet template object
            if( empty($this->Config['ContentType']) )
                $this->Config['ContentType'] = 'text/html; charset=UTF-8';

            header("Content-Type: {$this->Config['ContentType']}");
        }

        // apply any app-wide directives
        foreach( $this->Manifest['Directives'] as $V )
        {
            if( empty($this->{$V[0]}) )
                throw new Exception("Directive object {$V[0]}' doesn't exist while executing app for '{$this->Config['Hostname']}'.");
            else
                $this->{$V[0]}->ApplyDirective($V[1],$V[2]);
        }

        // optionally execute a function just prior to the page and rendering
        // if this function returns FALSE things are left to the function and nothing further is done
        if( !empty($this->Config['PreFunction']) )
        {
            if( $this->Config['PreFunction']($this) !== FALSE )
            {
                // now execute the actual page(s)
                if( !empty($this->OrderedMatch) )
                    $this->ps->Execute($this->OrderedMatch);

                if( !empty($this->ExactMatch) )
                    $this->ps->Execute($this->ExactMatch);

                // and finally begin rendering at the Base.tpl template
                $this->html->Base();
            }
        }
        // otherwise just execute the page and render
        else
        {
            // now execute the actual page(s)
            if( !empty($this->OrderedMatch) )
                $this->ps->Execute($this->OrderedMatch);

            if( !empty($this->ExactMatch) )
                $this->ps->Execute($this->ExactMatch);

            // and finally begin rendering at the Base.tpl template
            $this->html->Base();
        }
    }


    /**
     * Generic debug toggler for pages and templates.
     */
    public function Debugger()
    {
        // if we're not caching the manifest we know we're not in production, so implement
        // some easy toggling of debugging pages and templates and tweak our links to persist
        if( empty($this->Manifest['CacheManifest']) || !empty($_GET['or']) )
        {
            if( !empty($_GET['debug']) )
            {
                $this->lp->SetBaseURL($this->Request['SiteURL'],$_GET);

                $this->ps->DebugOn();
                $this->html->DebugOn();
            }
        }
    }


    /**
     * Build the application's manifest from a Google Spreadsheet.
     *
     * A manifest URL typically looks something like:
     *   https://docs.google.com/spreadsheet/ccc?key=SOME_STRING_OF_CHARS&usp=docslist_api
     *
     * @param string $ManifestURL URL to load Google spreadsheet from.
     * @retval array The application's manifest.
     */
    protected function BuildManifest( $ManifestURL )
    {
        $AppRoot = Path::Init($this->AppRoot);
        $AppRoot['IsDir'] = TRUE;
        $AppRoot = Path::ToString($AppRoot);

        // Pages/PageMaps are numeric arrays of one of more page sets
        // Directives are app-wide directives executed for every request.
        $Manifest = array('AppRoot'=>$AppRoot,'Config'=>array(),'Directives'=>array(),
                'Pages'=>array(),'PageMaps'=>array(),'Templates'=>array());

        // TODO: filesystem manifest.gsheet will likely be deprecated
        if( empty($ManifestURL) )
        {
            if( !is_readable($AppRoot.'manifest.gsheet') )
                throw new Exception("Can't read {$AppRoot}manifest.gsheet and no ManifestURL provided.");

            // pull the main URL from our local gsheet file
            $ManifestURL = file_get_contents($AppRoot.'manifest.gsheet');
            $ManifestURL = json_decode($ManifestURL,TRUE);
            $ManifestURL = $ManifestURLOrig =  URL::Init($ManifestURL['url']);
        }
        else
        {
            $ManifestURL = $ManifestURLOrig = URL::Init($ManifestURL);
        }

        // munge to be exportable as html
        $ManifestURL['Path']['Segments'][1] = 'pub';
        $ManifestURL['Query']['output'] = 'html';
        $ManifestURL['Query']['usp'] = NULL;

        // read in the HTML version which we scrape for each of the tabs
        $Buf = file_get_contents(URL::ToString($ManifestURL));

        $Headers = \asm\restr::ParseHeaders($http_response_header);

        if( empty($Headers['http']) || strpos($Headers['http'],'200') === FALSE )
            throw new Exception("manifest.gsheet doesn't appear published in $AppRoot");

        libxml_use_internal_errors(TRUE);
        $DOM = new \DOMDocument;
        $DOM->strictErrorChecking = FALSE;
        $DOM->preserveWhiteSpace = TRUE;
        $DOM->formatOutput = TRUE;
        $DOM->xmlStandalone = TRUE;
        $DOM->recover = TRUE;
        $DOM->resolveExternals = FALSE;
        @$DOM->loadHTML($Buf);

        $XPR = new \DOMXPath($DOM);

        // hack because of google: the first tab is the original URL which has no gid
        $Tabs = array('FIRST'=>URL::ToString($ManifestURL));

        $R = $XPR->query('//ul[@id="sheettabs"]/li/a');

        foreach( $R as $V )
        {
            // tabs must be uniquely named
            $Tabs[$V->textContent] = $V->getAttribute('href');
        }

        // go over each tab and pull as CSV and parse into our manifest
        foreach( $Tabs as $Label => $T )
        {
            // skip documentation tab
            if( strtolower($Label) === 'reference' )
                continue;

            $ManifestURL = URL::Init($T);
            // munge to get CSV
            $ManifestURL['Query']['output'] = 'csv';

            $B = @file(URL::ToString($ManifestURL));

            $Headers = \asm\restr::ParseHeaders($http_response_header);

            // something isn't right so just skip (not 200 or not csv)
            if( empty($Headers['http']) || strpos($Headers['http'],'200') === FALSE
             || empty($Headers['content-type']) || strpos($Headers['content-type'],'text/csv') === FALSE )
                continue;

            // config tab - Key, Value
            if( strpos($B[0],'Key,Value') !== FALSE )
            {
                array_shift($B);
                foreach( $B as $L )
                {
                    $L = str_getcsv($L);

                    if( empty($L[0]) )
                        continue;
                    else if( $L[0][0] === '$' )
                        $Manifest['Directives'][] = array(str_replace('$','',$L[0]),$L[1],$L[2]);
                    else if( $L[0] === 'AppFunction' )
                        $Manifest['Config']['AppFunction'] = $this->ParseFunctionName($L[1]);
                    else
                    {
                        // trim and normalize to boolean/NULL
                        $Manifest['Config'][$L[0]] = trim($L[1]);
                        $T = strtolower($Manifest['Config'][$L[0]]);

                        if( $T === 'false' )
                            $Manifest['Config'][$L[0]] = FALSE;
                        else if( $T === 'true' )
                            $Manifest['Config'][$L[0]] = TRUE;
                        else if( $T === 'null' )
                            $Manifest['Config'][$L[0]] = NULL;
                    }
                }
            }
            // pages tab - Name, Path, Status, Function, Directives
            // a 6th column in the header is required as a unique token for the page set (will clobber)
            else if( strpos($B[0],'Name,Path,Status') !== FALSE )
            {
                $L = array_shift($B);
                $L = str_getcsv($L);
                if( empty($L[5]) )
                {
                    trigger_error('Skipping invalid Page header: '.implode(',',$L));
                    continue;
                }

                $PageSetName = trim($L[5]);

                $LastPage = '';
                $Page = $PageMap = array();
                foreach( $B as $L )
                {
                    $L = str_getcsv($L);
                    if( !empty($L[0]) )
                    {
                        $P = Page::Init($L[0],$L[1],$L[2],$this->ParseFunctionName($L[3]));
                        $Page[$P['Name']] = $P;
                        $PageMap[$P['Path']] = $P['Name'];
                        $LastPage = $P['Name'];
                    }

                    if( !empty($L[4]) )
                    {
                        if( !empty($L[5]) )
                            $D = array($L[4],$L[5],$L[6]);
                        else
                            $D = explode(',',$L[4]);

                        if( !empty($LastPage) )
                            $Page[$LastPage]['Directives'][] = $D;
                    }
                }

                // duplicate/empty PageSetNames will overwrite
                $Manifest['Pages'][$PageSetName] = $Page;
                $Manifest['PageMaps'][$PageSetName] = $PageMap;
            }
            // templates tab - Name, Function - we just store the name/function mapping and the rest is init in the AppFile
            // name must contain a prefix if the template file is in a sub-directory
            else if( strpos($B[0],'Name,Function') !== FALSE )
            {
                array_shift($B);
                foreach( $B as $L )
                {
                    $L = str_getcsv($L);
                    if( empty($L[0]) )
                        continue;
                    else
                        $Manifest['Templates'][$L[0]] = $this->ParseFunctionName($L[1]);
                }
            }
        }

        // if no config something isn't right
        if( empty($Manifest['Config']) )
            throw new Exception("Couldn't find a config tab at '".URL::ToString($ManifestURLOrig));

        return $Manifest;
    }

    // if CacheApp isn't TRUE, we leave templates on disk and will include them - this helps with debugging
    // since PHP's error messages directly point to a line and file - see also Template::__call()
    // only .php and .inc files are pulled in - .tpl and .html and .js and .css for templates
    protected function BuildFile()
    {
        $AppRoot = $this->Manifest['AppRoot'];

        if( !is_dir($AppRoot) || !is_readable($AppRoot) )
            throw new Exception("Unreadable AppRoot '{$AppRoot}'");

        $Hostname = $this->Hostname;

        $AppFile = '';

        // PHP Bug - can't use FilesystemIterator::CURRENT_AS_PATHNAME |  FilesystemIterator::KEY_AS_FILENAME with recursive dirs
        $Flags = \FilesystemIterator::SKIP_DOTS | \FilesystemIterator::UNIX_PATHS;

        // subdirectories are NOT prefixed
        $dir = new \RecursiveDirectoryIterator("{$AppRoot}lib",$Flags);
        $fs = new \RecursiveIteratorIterator($dir);
        foreach( $fs as $K => $V )
        {
            $PI = pathinfo($K);
            if( in_array(strtolower($PI['extension']),array('php','inc')) === FALSE )
                continue;

            // if we're not caching, just include the file
            // this helps tremendously with useful error messages - ditto below as well
            if( empty($this->CacheApp) )
            {
                include $K;
            }
            else
            {
                $AppFile .= "\n\n\n/*** {$K} ***/";

                $T = php_strip_whitespace($K);
                if( strpos($T,'<?php') === 0 )
                    $AppFile .= "\n".substr($T,5);
                else
                    $AppFile .= $T;
            }
        }

        // subdirectories are NOT prefixed
        $dir = new \RecursiveDirectoryIterator("{$AppRoot}functions",$Flags);
        $fs = new \RecursiveIteratorIterator($dir);
        foreach( $fs as $K => $K )
        {
            $PI = pathinfo($K);
            if( in_array(strtolower($PI['extension']),array('php','inc')) === FALSE )
                continue;

            if( empty($this->CacheApp) )
            {
                include $K;
            }
            else
            {
                $AppFile .= "\n\n\n/*** {$K} ***/";

                $T = php_strip_whitespace($K);
                if( strpos($T,'<?php') === 0 )
                    $AppFile .= "\n".substr($T,5);
                else
                    $AppFile .= $T;
            }
        }

        // subdirectories ARE prefixed and merge any Function definition from the manifest
        $AppFile .= "\n\n\n/*** {$AppRoot}templates ***/";
        $dir = new \RecursiveDirectoryIterator("{$AppRoot}templates",$Flags);
        $fs = new \RecursiveIteratorIterator($dir);
        $Templates = array();
        foreach( $fs as $K => $V )
        {
            $PI = pathinfo($K);
            if( in_array(strtolower($PI['extension']),array('php','inc','tpl','html','css','js')) === FALSE )
                continue;

            $P = Path::Init($K);
            $P = Path::Bottom($P,2);

            if( $P['Segments'][0] !== 'templates' )
                $Prefix = $P['Segments'][0].'_';
            else
                $Prefix = '';

            $Buf = file_get_contents($K);

            if( strpos(substr($Buf,0),"\n@@@") === FALSE )
            {
                $P['Segments'][1] = pathinfo($P['Segments'][1],PATHINFO_FILENAME);

                if( !empty($this->Manifest['Templates'][$Prefix.$P['Segments'][1]]) )
                    $F = $this->Manifest['Templates'][$Prefix.$P['Segments'][1]];
                else
                    $F = array();

                // if we're not caching, don't store the body - it'll be include()'d in TemplateSet::__call()
                // if we are, prepend the start tag so that it can be eval()'d without string munging
                // same for below
                if( empty($this->CacheApp) )
                    $Buf = '';
                else
                    $Buf = "?>{$Buf}";

                $Templates[$Prefix.$P['Segments'][1]] = Template::Init($Prefix.$P['Segments'][1],$K,$F,$Buf);
            }
            else
            {
                // this regex needs to be more robust, including handling empty frags, frags that don't
                // start with whitespace, those that end with a comment, etc. - these are always set in the body
                // and never have a path
                // the naming scheme here is DirName_Filename or just Filename
                $B = preg_split("/\s*@@@(\w+[a-zA-Z0-9\-]*)/m",$Buf,0,PREG_SPLIT_DELIM_CAPTURE|PREG_SPLIT_NO_EMPTY);
                $CNT = count($B);
                for( $i = 0; $i < $CNT; $i+=2 )
                {
                    if( !empty($this->Manifest['Templates'][$Prefix.$B[$i]]) )
                        $F = $this->Manifest['Templates'][$Prefix.$B[$i]];
                    else
                        $F = array();

                    $Templates[$Prefix.$B[$i]] = Template::Init($Prefix.$B[$i],'',$F,"?>{$B[$i+1]}");
                }
            }
        }

        return "<?php\n\n{$AppFile} \n\n \$this->Templates = ".var_export($Templates,TRUE).';';
    }


    protected function ParseFunctionName( $F )
    {
        if( strpos($F,'::') === FALSE )
        {
            return trim(str_replace(array('(',')'),'',$F));
        }
        else
        {
            $T = explode('::',$F);
            return array(trim($T[0]),trim(str_replace(array('(',')'),'',$T[1])));
        }
    }


    /**
     * Generic handler for when no pages could be matched.
     *
     * This method should be overridden in the application's extending App class.
     */
    public function NoPageHandler()
    {
        Log::Log('NO PAGE FOUND: '.URL::ToString($this->Request));

        if( $this->Request['IsCLI'] === FALSE )
            \asm\HTTP::_404();

        exit;
    }


    /**
     * User error handler.
     *
     * Handle an application or PHP error.  This method is set using
     * set_error_handler() in App::__construct().
     *
     * Framewire uses PHP's native error handling, allowing well-known
     * PHP functions such as trigger_error() to intelligently deliver
     * error messages using advanced mechanisms, such as Wildfire or
     * email.
     *
     * This method maps PHP's error constants to Framewire's Log error
     * levels.
     *
     * @param int $errno The message severity as a PHP constant.
     * @param string $errstr The error message.
     * @param string $errfile The filename from which the message came.
     * @param int $errline The line number of the file.
     * @param array $errcontext Local scope where the error occurred.
     * @retval boolean FALSE if the error should be ignored, TRUE if not.
     *
     * @todo errcontext can be huge - might filter/etc
     */
    public function ErrorHandler( $errno,$errstr,$errfile,$errline,$errcontext )
    {
        // error surpressed with @
        if( error_reporting() === 0 )
            return FALSE;

        if( in_array($errno,array(E_WARNING,E_USER_WARNING)) )
            $errno = 'WARN';
        else if( in_array($errno,array(E_NOTICE,E_USER_NOTICE)) )
            $errno = 'INFO';
        else
            $errno = 'ERROR';

        $BT = array_merge(array("[{$errfile}:{$errline}]"),Debug::Backtrace());

        Log::Log($errstr,$errno,$BT,$errcontext);

        return TRUE;
    }

    /**
     * Set PHP's open_basedir() directive for the application.
     *
     * @param string $Path Base directory.
     * @todo implement/test - currently a no op
     */
    public function OpenBaseDir( $Path = '' )
    {
        if( empty($Path) )
        {
            // these defaults need work - should use cache directory and AppRoot or something
            // so no this is a no op
            return;

            if( Instance::IsWindows() )
                ini_set('open_basedir',ASM_ROOT.';C:/Windows/Temp/');
            else
                ini_set('open_basedir',ASM_ROOT.':/tmp');
        }
        else
            ini_set('open_basedir',$Path);
    }

    /**
     * Determine whether we're executing in a Windows environment.
     *
     * @retval boolean TRUE if the application is executing in a Windows environment.
     * @todo Should PHP_OS be used instead?
     */
    public static function IsWindows()
    {
        return isset($_SERVER['SystemRoot']);
    }

}
