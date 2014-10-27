<?php
/**
 * @file App.php asmblr application controller.
 * @author Stackware, LLC
 * @version 4.2
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * Base configuration, controller and runtime container for an application.  The App class
 * contains key system start-up logic and methods, including handling 404 and server errors.
 *
 * It must be extended and customized in an application's @c Load.inc.
 *
 * @note This is your GOD anti-pattern and includes dynamic properties.  The
 *       mess goes here, so the rest is clean.
 */
abstract class App
{
    /**
     * @var string $CacheDir
     * Absolute path to the asmblr local cache.
     *
     * @note This must be set explicitly in the extending class.
     *
     * @todo For future GAE integration:
     *      @c gs://asmblr-mc-tmp/
     *      @c array('gs'=>array('Content-Type'=>'text/plain'))
     */
    public $CacheDir = '';

    /**
     * @var string $Hostname
     * The hostname of the application that has matched the request.
     *
     * @note This is the requested hostname as matched from @c index.php
     */
    public $Hostname = '';

    /**
     * @var string $AppRoot
     * The absolute path of the application's root directory, for example:
     * @c /var/www/aapps/example.com
     *
     * Many other paths are relative to this path.
     *
     * @note This is configured in the application array in @c index.php
     */
    public $AppRoot = '';

    /**
     * @var boolean $CacheManifest
     * TRUE to cache the manifest to the local disk.
     *
     * @note This is configured in the application array in @c index.php
     */
    public $CacheManifest = FALSE;

    /**
     * @var boolean $CacheApp
     * TRUE to cache the app into a local app-file.
     *
     * @note This is configured in the application array in @c index.php
     */
    public $CacheApp = FALSE;

    /**
     * @var string $RoutingPS
     * The PageSet token (cell @c F1) of the manifest page tab that will be used for processing the current request.
     *
     * @note This is configured in the application array in @c index.php
     * @note Read-only.
     */
    public $RoutingPS = '';

    /**
     * @var Page $OrderedMatch
     * The Page Struct that that matched a hierarchal path (those with a trailing @c / in the URL) ie., an ordered match.
     *
     * @note Read-only.  May be empty.
     */
    public $OrderedMatch;

    /**
     * @var Page $ExactMatch
     * The Page Struct that was an exact match (those withot a trailing @c / in the URL) that matched the request.
     *
     * @note Read-only.  May be empty.
     */
    public $ExactMatch;

    /**
     * @var string $ClosestMatchName
     * The Page name of the page that most closely matches the request.
     *
     * If there is an exact match, this will be the same as App::$ExactMatch, otherwise it will
     * be App::$OrderedMatch.
     *
     * @note Read-only.  May be empty only if there's no matches (404).
     */
    public $ClosestMatchName;

    /**
     * @var asm::Request $Request
     * The Request Struct which contains normalized details about the current HTTP or CLI request.
     *
     * @note Read-only.
     */
    public $Request;

    /**
     * @var array $Manifest
     * The application's manifest built from it's Google Spreadsheet.
     *
     * The manifest contains application configuration, including pages/routes, templates and misc.
     * configuration settings.
     *
     * @note Read-only.
     */
    public $Manifest;

    /**
     * @var array $Pages
     * A listing page Pages by name.
     *
     * This contains pages for each PageSet tab defined by the manifest.
     *
     * @note Read-only.
     */
    public $Pages;

    /**
     * @var array $PageMaps
     * A mapping of Page paths (URL) to their Page Name.
     *
     * This contains all page mappings; one for each PageSet tab defined by the manifest.
     *
     * @note Read-only.
     */
    public $PageMaps;

    /**
     * @var array $Config
     * Key/value config directives defined by the Config tab in the manifest.
     *
     * A key with two or more values (each it's own column) will have an array value.
     *
     * @note Read-only.
     */
    public $Config;

    /**
     * @var array $Templates
     * A listing of Templates as found in the @c templates directory and defined by the manifest.
     *
     * @note Read-only.
     */
    public $Templates;


    /**
     * Application build and boot.
     *
     * Here the manifest and application is built, loaded, and optionally cached.  This method must be
     * called explicitly by the extending class if not fully re-implemented.
     *
     * Key system objects are also instantiated as properties of the object:
     *  - @c $this->ps = PageSet
     *  - @c $this->lp = LinkPage
     *  - @c $this->lc = Linkcnvyr
     *
     * @param array $App Application array defined in @c index.php.
     * @param asm::Request $Request Request Struct to use as the current request.
     * @param array $ConfigOverride Key/value array of custom application config variables.
     *
     * @throws Exception CacheDir not set.
     *
     * @note $Request can be @em spoofed if needed.
     * @note If $App contains a Dirs element (defined by the application array in @c index.php),
     *       it's used by BuildFile, rather than the default directory structure.
     * @note No locking of cache files is performed.
     *
     * @todo Do we want a DataSheets property?
     * @todo Fully test that app-file caching works properly with complex apps/manifests/datasheets/etc.
     */
    public function __construct( $App,$Request,$ConfigOverride = array() )
    {
        $this->Hostname = $App['Hostname'];
        $this->AppRoot = $App['AppRoot'];
        $this->CacheManifest = $App['CacheManifest'];
        $this->CacheApp = $App['CacheApp'];
        $this->RoutingPS = $App['RoutingPS'];

        $this->Request = $Request;

        if( empty($this->CacheDir) )
            throw new Exception('CacheDir not set.');

        // make this object global :)
        $GLOBALS['asmapp'] = $this;

        // build/cache our manifest
        if( $this->CacheManifest === TRUE )
        {
            if( (@include "{$this->CacheDir}{$this->Hostname}.manifest.inc") === FALSE )
            {
                $this->Manifest = $this->BuildManifest($App['ManifestURL']);
                file_put_contents("{$this->CacheDir}{$this->Hostname}.manifest.inc",'<?php $this->Manifest = '.var_export($this->Manifest,TRUE).';');
            }
        }
        else
        {
            // clear an existing cache - defaults to above path
            $this->ClearManifestCache();
            $this->Manifest = $this->BuildManifest($App['ManifestURL']);
        }

        // config variables defined in the manifest's Config tab (even if empty) can be
        // overridden in the application array
        // note that these values aren't cached (is this slow?)
        foreach( array_intersect_key($App,$this->Manifest['Config']) as $K => $V )
            $this->Manifest['Config'][$K] = $V;

        // finally apply hard-overrides or custom config variables not present in the manifest
        foreach( $ConfigOverride as $K => $V )
            $this->Manifest['Config'][$K] = $V;

        // set local properties for the manifest - would like to streamline as it's wasteful
        // but we'd need to refactor how we pull from the cache
        $this->Config = $this->Manifest['Config'];
        $this->Pages = $this->Manifest['Pages'];
        $this->PageMaps = $this->Manifest['PageMaps'];

        if( !empty($App['Dirs']) )
            $Dirs = $App['Dirs'];
        else
            $Dirs = array();

        // build/cache our app file
        if( $this->CacheApp === TRUE )
        {
            // this will set $this->Templates if already cached
            if( (@include "{$this->CacheDir}{$this->Hostname}.app.inc") === FALSE )
            {
                file_put_contents("{$this->CacheDir}{$this->Hostname}.app.inc",$this->BuildApp($Dirs));
                include "{$this->CacheDir}{$this->Hostname}.app.inc";
            }
        }
        else
        {
            // clear an existing cache - defaults to above path
            $this->ClearAppCache();
            $T = $this->BuildApp($Dirs);
            eval('?>'.$T);
        }

        if( empty($this->Config['Status']) || $this->Config['Status'] !== 'Active' )
            HTTP::_400();

        // activate asmblr's stock error handler - method can be customized in extending class
        set_error_handler(array($this,'ErrorHandler'));

        // Calculate app-wide URLs and request info based on the current request
        Request::CalcURLs($this->Request,$this->Config['BaseURL']);


        // instantiate core system objects
        // this sets dynamic properties - know your names

        // multiple PageSets are supported though only a single PageSet will be the RoutingPS
        // this PageSet will be used for routing and link creation
        // other pagesets/linkers can be instantiated manually if needed
        // note that we use isset because an empty routing PS is ok
        if( empty($this->RoutingPS) || !isset($this->Pages[$this->RoutingPS]) )
            throw new Exception("Invalid routing page set {$this->RoutingPS} for '{$this->Hostname}'");

        $this->ps = new PageSet($this->Pages[$this->RoutingPS],$this->PageMaps[$this->RoutingPS]);
        $this->lp = new LinkPage($this->ps,$this,$this->Request['SiteURL']);

        // links for cnvyr managed resources (images, css, js, fonts/etc)
        $this->lc = new Linkcnvyr($this->ps,$this,$this->Request['SiteURL']);
    }

    /**
     * Match pages, apply app directives, execute SitewideFunction, execute pages, and render templates.
     *
     * This enforces the @c ForceBaseHostname config setting if not running as a CLI.
     *
     * @throws Exception Directive object doesn't exist.
     *
     * @note This uses a lowercased MatchPath.
     * @note If an ordered match is found and doesn't have a Status of Weak, no exact match will be attempted.
     * @note The SitewideFunction, if present, will know the pages that have matched the request and can rearrange as needed.
     * @note If @c SitewideFunction returns FALSE, default page execution and rendering will not happen,
     *       including 404 and Page status checks.
     */
    public function Execute()
    {
        // if not running as a CLI, first honor our ForceBaseHostname setting
        if( $this->Request['IsCLI'] === FALSE && ($this->Config['ForceBaseHostname'] === TRUE && $this->Request['IsBaseHostname'] === FALSE) )
        {
            $this->Request['Hostname'] = $this->Request['BaseURL']['Hostname'];
            HTTP::Location($this->Request);
        }

        // match pages against the MatchPath to determine our executing page(s)
        $this->OrderedMatch = $this->ExactMatch = array();
        $this->ClosestMatchName = '';

        // we'll use a copy of MatchPath and have it lowercased
        $MatchPath = $this->Request['MatchPath'];
        Path::Lower($MatchPath);

        // first determine hierarchal ordered matches - most general to most specific URL path
        // only one page will match
        if( $MatchPath['IsRoot'] === FALSE )
        {
            foreach( \asm\Path::Order($MatchPath) as $V )
            {
                if( ($this->OrderedMatch = $this->ps->Match($V)) !== NULL )
                {
                    $this->ClosestMatchName = $this->OrderedMatch['Name'];
                    break;
                }
            }
        }

        // if an ordered match isn't found, determine an exact match.  only one page would match
        // if the ordered match is Weak, an exact match is also allowed
        // i.e.  default:  /admin/ matches then a page with /admin/something will NOT match
        //          Weak:  /admin/ matches then a page with /admin/something WILL match
        if( empty($this->OrderedMatch) || (strpos($this->OrderedMatch['Status'],'Weak') !== FALSE) )
        {
            if( ($this->ExactMatch = $this->ps->Match(\asm\Path::ToString($MatchPath))) !== NULL )
                $this->ClosestMatchName = $this->ExactMatch['Name'];
        }

        // apply app-wide directives
        foreach( $this->Manifest['Directives'] as $V )
        {
            if( empty($this->{$V[0]}) )
                throw new Exception("Directive object '{$V[0]}' doesn't exist while executing app for '{$this->Config['Hostname']}'.");
            else
                $this->{$V[0]}->ApplyDirective($V[1],$V[2]);
        }

        // execute SitewideFunction - returning FALSE will bypass default page checking, execution and HTML rendering, below
        if( !empty($this->Config['SitewideFunction']) )
            $PreContinue = $this->Config['SitewideFunction']($this);
        else
            $PreContinue = TRUE;

        // returning FALSE from SiteWideFunction above will bypass default page checking, execution and HTML rendering
        if( $PreContinue !== FALSE )
        {
            // if no pages, it's a 404
            if( empty($this->OrderedMatch) && empty($this->ExactMatch) )
                $this->NoPageHandler();

            // or if one isn't active
            if( (!empty($this->OrderedMatch['Status']) && (strpos($this->OrderedMatch['Status'],'Active') === FALSE))
            || (!empty($this->ExactMatch['Status']) && ($this->ExactMatch['Status'] !== 'Active')) )
                $this->NoPageHandler();

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
     * Default debug toggler for pages and templates.
     *
     * This helper method activates debugging for the PageSet (@c $this->lp) and TemplateSet
     * (@c $this->html) based on the presence of the @c debug query string parameter.
     *
     * Debugging is only allowed if the manifest is @e not being cached, or if the @c or
     * query string parameter is present and matches the @c $this->RoutingPS value.
     *
     * If the @c display query string parameter is set to @c 0 then debug output won't be
     * output to the browser.
     */
    public function Debugger()
    {
        // if we're not caching the manifest we know we're not in production, so implement
        // some easy toggling of debugging pages and templates and tweak our links to persist
        // by default we enable browser display, though this can be surpressed with display=0
        if( empty($this->Manifest['CacheManifest']) || !empty($_GET['or']) )
        {
            if( !empty($_GET['debug']) )
            {
                $this->lp->SetBaseURL($this->Request['SiteURL'],$_GET);

                if( isset($_GET['display']) && $_GET['display'] === '0' )
                    $Display = FALSE;
                else
                    $Display = TRUE;

                $this->ps->DebugOn($Display);
                $this->html->DebugOn($Display);
            }
        }
    }


    /**
     * Default handler for when no pages could be matched (a 404).
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
     * Set PHP's open_basedir() directive for the application's execution.
     *
     * @param string $Path Base directory
     *
     * @todo This isn't currently used anywhere.  Also needs to consider cache directories
     *       and the AppRoot.
     * @todo Needs finalization - only for shared environments.
     */
    public function OpenBaseDir( $Path = '' )
    {
        if( empty($Path) )
        {
            if( Instance::IsWindows() )
                ini_set('open_basedir',ASM_ROOT.';C:/Windows/Temp/');
            else
                ini_set('open_basedir',ASM_ROOT.':/tmp');
        }
        else
            ini_set('open_basedir',$Path);
    }


    /**
     * Delete the manifest cache file if it exists.
     *
     * @param string $P The absolute path to the cache file or empty for the default location.
     *
     * @note Careful - this can unlink files.
     */
    public function ClearManifestCache( $P = '' )
    {
        if( empty($P) )
            $P = "{$this->CacheDir}{$this->Hostname}.manifest.inc";

        @unlink($P);
    }


    /**
     * Delete the app cache file if it exists.
     *
     * @param string $P The absolute path to the cache file or empty for the default location.
     *
     * @note Careful - this can unlink files.
     */
    public function ClearAppCache( $P = '' )
    {
        if( empty($P) )
            $P = "{$this->CacheDir}{$this->Hostname}.app.inc";

        @unlink($P);
    }


    /**
     * Determine whether we're executing in a Windows environment.
     *
     * @retval boolean TRUE if the application is executing in a Windows environment.
     */
    public static function IsWindows()
    {
        return isset($_SERVER['SystemRoot']);
    }


    /**
     * Build the application's manifest from a Google Spreadsheet.
     *
     * A manifest URL will look something like:
     *   https://docs.google.com/spreadsheets/d/SOME_STRING_CHARS/pubhtml
     *
     * @param string $ManifestURL The URL of the Google Spreadsheet.
     * @retval array The application's manifest.
     */
    protected function BuildManifest( $ManifestURL )
    {
        // only new google spreadsheets in asmblr 4.2
        $Manifest = $this->Manifestd($ManifestURL);

        return $Manifest;
    }


    /**
     * Extract the tabs of a Google Spreadsheet as CSV strings.
     *
     * @param string $ManifestURL The URL of the Google Spreadsheet.
     * @throws Exception Manifest doesn't appear published.
     * @throws Exception Couldn't find a config tab.
     * @retval array The application's manifest.
     */
    protected function Manifestd( $ManifestURL )
    {
        // read in the HTML version which we scrape for each of the tabs
        $Buf = file_get_contents($ManifestURL);
        $Headers = \asm\restr::ParseHeaders($http_response_header);

        if( empty($Headers['http']) || strpos($Headers['http'],'200') === FALSE )
            throw new Exception("Manifest doesn't appear published in {$this->AppRoot}");

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

        $Tabs = array();
        $R = $XPR->query('//ul[@id="sheet-menu"]/li/a');

        foreach( $R as $V )
        {
            // tabs must be uniquely named
            $Tabs[$V->textContent] = str_replace(array('switchToSheet(\'','\')'),'',$V->getAttribute('onclick'));
        }

        foreach( $Tabs as $Name => $ID )
        {
            // may be handy someday
            // $TabCSVd = fopen('php://temp','rw');

            $Tab = array();

            $R = $XPR->query("//div[@id=\"{$ID}\"]//tbody//tr");
            foreach( $R as $K => $V )
            {
                $Line = array();
                foreach( $V->getElementsByTagName('td') as $K2 => $V2 )
                {
                    $Line[] = "\"{$V2->textContent}\"";
                }

                // will break if value contains comma
                $Tab[] = implode(',',$Line);
            }

            $Tabs[$Name] = $Tab;
        }

        // Pages/PageMaps are numeric arrays of one of more page sets
        // Directives are app-wide directives executed for every request.
        // DataSheets are an associative array - by sheet name - of general purpose data (key/value)
        $Manifest = array('AppRoot'=>$this->AppRoot,'Config'=>array(),'Directives'=>array(),
                            'Pages'=>array(),'PageMaps'=>array(),'Templates'=>array(),'DataSheets'=>array());

        // now actually build the manifest from the CSV tabs
        foreach( $Tabs as $Label => $B )
        {
            $Tab = $this->ParseTabCSV($B);

            // some elements may be empty, but that's ok so use isset
            if( isset($Tab['Config']) )
            {
                $Manifest['Config'] = $Tab['Config'];
                $Manifest['Directives'] = $Tab['Directives'];
            }
            else if( isset($Tab['PageSetName']) )
            {
                $Manifest['Pages'][$Tab['PageSetName']] = $Tab['Pages'];
                $Manifest['PageMaps'][$Tab['PageSetName']] = $Tab['PageMaps'];
            }
            else if( isset($Tab['Templates']) )
            {
                $Manifest['Templates'] = $Tab['Templates'];
            }
            else if( isset($Tab['DataSheet']) )
            {
                $Manifest['DataSheets'][$Label] = $Tab['DataSheet'];
            }
            else
            {
                trigger_error("Skipping unknown tab structure with name '{$Label}'");
            }
        }

        // if no config something isn't right
        if( empty($Manifest['Config']) )
            throw new Exception("Couldn't find a config tab for {$this->AppRoot}.");

        return $Manifest;
    }


    /**
     * Parse an array of CSV lines from a Google Spreadsheet tab into a manifest section.
     *
     * @param array $B An array of CSV lines.
     * @retval array A normalized manifest section.
     */
    protected function ParseTabCSV( $B )
    {
        // config tab - Key, Value
        // directives must have a $ in the key column (column 0) - comma notation isn't currently supported
        // multiple columns for a single key will cause the key to be an array containing all values
        if( strpos($B[0],'"Key","Value"') !== FALSE )
        {
            $Tab = array('Config'=>array(),'Directives'=>array());

            array_shift($B);
            foreach( $B as $L )
            {
                // skip comments
                if( substr(trim($L,', "'),0,2) === '//' )
                    continue;

                $L = str_getcsv($L);

                // always trim whitespace
                foreach( $L as $K => $V )
                    $L[$K] = trim($V);

                // remove empty columns so our column count is accurate - a bit hacky but works for now
                $L2 = array();
                foreach( $L as $V )
                {
                    if( $V === '' )
                        continue;
                    else
                        $L2[] = trim($V);
                }

                $L = $L2;

                // skip blank lines
                if( implode('',$L) === '' )
                    continue;

                // a directive in the config tab is recognized by having a $ as the first char of the key
                // comma notation isn't currently supported in the config tab
                if( $L[0][0] === '$' )
                {
                    $Tab['Directives'][] = array(str_replace('$','',$L[0]),$L[1],$L[2]);
                }
                // otherwise taken as a key/value config parameter and trim/normalize
                // a blank column 0 rolls the value into the previous key which is converted to an array
                else
                {
                    if( $L[0] === 'SitewideFunction' )
                    {
                        $Tab['Config']['SitewideFunction'] = $this->ParseFunctionName($L[1]);
                    }
                    else
                    {
                        $Tab['Config'][$L[0]] = '';

                        // support multiple values as multiple columns
                        for( $i = 1; $i < 20; ++$i )
                        {
                            // trim and normalize to boolean/NULL if a value was set.
                            if( isset($L[$i]) )
                            {
                                $V = trim($L[$i]);
                                $V2 = strtolower($V);

                                if( $V2 === 'false' )
                                    $V = FALSE;
                                else if( $V2 === 'true' )
                                    $V = TRUE;
                                else if( $V2 === 'null' )
                                    $V = NULL;

                                if( $i > 1 && is_array($Tab['Config'][$L[0]]) === FALSE )
                                    $Tab['Config'][$L[0]] = array($Tab['Config'][$L[0]],$V);
                                else if( $i > 1 )
                                    $Tab['Config'][$L[0]][] = $V;
                                else
                                    $Tab['Config'][$L[0]] = $V;
                            }
                        }
                    }
                }
            }

            return $Tab;
        }
        // pages tab - Name, Path, Status, Function, Directives
        // a 6th column in the header is required as a unique token for the page set (will clobber)
        else if( strpos($B[0],'"Name","Path","Status"') !== FALSE )
        {
            $L = array_shift($B);

            $L = str_getcsv($L);

            if( empty($L[5]) )
            {
                trigger_error('Skipping invalid Page header - no PageSet Name: '.implode(',',$L));
                return array();
            }

            $PageSetName = trim($L[5]);

            $LastPage = '';
            $Page = $PageMap = array();
            foreach( $B as $L )
            {
                // skip comments
                if( substr(trim($L,', "'),0,2) === '//' )
                    continue;

                $L = str_getcsv($L);

                // always trim whitespace
                foreach( $L as $K => $V )
                    $L[$K] = trim($V);

                // have a full page entry
                if( !empty($L[0]) )
                {
                    $P = Page::Init($L[0],$L[1],$L[2],$this->ParseFunctionName($L[3]));
                    $Page[$P['Name']] = $P;
                    $PageMap[$P['Path']] = $P['Name'];
                    $LastPage = $P['Name'];
                }

                // have directives - if starts at column 4, then append to previous page
                if( !empty($L[4]) )
                {
                    if( !empty($L[5]) )
                        $D = array($L[4],$L[5],$L[6]);
                    else
                        $D = explode(',',$L[4]);

                    // squelch errors because sometime only two parameters and no trailing comma
                    // but this could also result in invalid directives
                    if( !empty($LastPage) )
                        $Page[$LastPage]['Directives'][] = array(str_replace('$','',@$D[0]),@$D[1],@$D[2]);
                }
            }

            return array('PageSetName'=>$PageSetName,'Pages'=>$Page,'PageMaps'=>$PageMap);
        }
        // templates tab - Name, Function - we just store the name/function mapping and the rest is init in the AppFile
        // name must contain a prefix if the template file is in a sub-directory
        else if( strpos($B[0],'"Name","Function"') !== FALSE )
        {
            $Tab = array('Templates'=>array());

            array_shift($B);
            foreach( $B as $L )
            {
                // skip comments
                if( substr(trim($L,', "'),0,2) === '//' )
                    continue;

                $L = str_getcsv($L);

                // always trim whitespace
                foreach( $L as $K => $V )
                    $L[$K] = trim($V);

                // skip blank lines
                if( empty($L[0]) )
                    continue;
                else
                    $Tab['Templates'][$L[0]] = $this->ParseFunctionName($L[1]);
            }

            return $Tab;
        }
        // data sheet tab - must have ID as first column - simply a set of key (column headers) and values (rows/cells)
        // ID is used if provided for each row, or the row number is used
        else if( strpos($B[0],'"ID"') === 0 )
        {
            $Tab = array('DataSheet'=>array());

            $Keys = str_getcsv($B[0]);

            array_shift($B);
            $LastID = '';
            foreach( $B as $RowID => $L )
            {
                // skip comments
                if( substr(trim($L,', "'),0,2) === '//' )
                    continue;

                $L = str_getcsv($L);

                // always trim whitespace
                foreach( $L as $K => $V )
                    $L[$K] = trim($V);

                // skip blank lines - this is a bit tricky/different for data sheets
                if( trim(implode('',$L)) === '' )
                    continue;

                // if both the first and second columns are empty, we'll fold the remaining columns
                // into the previous row's respective keys
                if( isset($L[0]) && isset($L[1]) && $L[0] === '' && $L[1] === '' )
                {
                    foreach( $Keys as $I => $V )
                    {
                        if( $I < 2 )
                            continue;

                        if( empty($L[$I]) )
                            continue;

                        if( isset($Tab['DataSheet'][$LastID][$V]) )
                        {
                            if( is_array($Tab['DataSheet'][$LastID][$V]) === FALSE )
                                $Tab['DataSheet'][$LastID][$V] = array($Tab['DataSheet'][$LastID][$V]);
                        }
                        // this probably never happens
                        else
                            $Tab['DataSheet'][$LastID][$V] = array();

                        $Tab['DataSheet'][$LastID][$V][] = $L[$I];
                    }
                }
                // new full row
                else
                {
                    if( $L[0] === '' )
                        $LastID = $ID = $RowID;
                    else
                        $LastID = $ID = $L[0];

                    foreach( $Keys as $I => $V )
                    {
                        if( $I === 0 )
                            continue;

                        $Tab['DataSheet'][$LastID][$V] = $L[$I];
                    }
                }
            }

            return $Tab;
        }

        return array();
    }


    /**
     * Compile an application into a single executeable string.
     *
     * This loads functions, templates and PHP code from the filesystem into memory as a single string.
     *
     * This will prefix Template names with their parent directory's name if more than one-level deep.  This
     * also parses templates for the \@\@\@ notation and is partially duplicated in TemplateSet::LoadFile
     *
     * If @c $Dirs is provided, it must be of the form:
     *   - @c array('lib'=>array(),'functions'=>array(),'templates'=>array())
     *
     * where each element is an array of directories to load from.
     *
     * @param array $Dirs An array of directories to load the application from.
     * @throws Exception Unreadable AppRoot
     * @retval string The compiled application.
     *
     * @note If App::$CacheApp is FALSE, the templates remain on disk and will be loaded by
     *       TemplateSet::LoadFile upon rendering.  This helps with debugging since PHP's error messages
     *       will reference a real file/line.
     * @note Only files with the following extensions are loaded: <tt> .inc .php .tpl .html .js .css </tt>
     */
    protected function BuildApp( $Dirs = array() )
    {
        if( empty($Dirs) )
        {
            $AppRoot = $this->Manifest['AppRoot'];

            if( !is_dir($AppRoot) || !is_readable($AppRoot) )
                throw new Exception("Unreadable AppRoot '{$AppRoot}'");

            $Dirs = array('lib'=>array("{$AppRoot}lib"),
                          'functions'=>array("{$AppRoot}functions"),
                          'templates'=>array("{$AppRoot}templates"));
        }

        $Hostname = $this->Hostname;

        $AppFile = '';

        // PHP Bug - can't use FilesystemIterator::CURRENT_AS_PATHNAME |  FilesystemIterator::KEY_AS_FILENAME with recursive dirs
        $Flags = \FilesystemIterator::SKIP_DOTS | \FilesystemIterator::UNIX_PATHS;

        // subdirectories are NOT prefixed
        foreach( $Dirs['lib'] as $D )
        {
            $dir = new \RecursiveDirectoryIterator($D,$Flags);
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
        }

        // subdirectories are NOT prefixed
        foreach( $Dirs['functions'] as $D )
        {
            $dir = new \RecursiveDirectoryIterator($D,$Flags);
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
        }

        // subdirectories ARE prefixed and merge any Function definition from the manifest
        // see also TemplateSet::Load()
        $Templates = array();
        foreach( $Dirs['templates'] as $D )
        {
            $AppFile .= "\n\n\n/*** {$D} ***/";
            $dir = new \RecursiveDirectoryIterator($D,$Flags);
            $fs = new \RecursiveIteratorIterator($dir);
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
                // we'd like to retire this (or at least only have in TemplateSet::LoadFile)
                //  but MC currently depends on it
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
        }

        return "<?php\n\n{$AppFile} \n\n \$this->Templates = ".var_export($Templates,TRUE).';';
    }


    /**
     * Helper method for normalizing and parsing a function string name into a callable
     * string or array.
     *
     * @param string $F The function name.
     * @retval string The normalized function name.
     * @retval array A Class:Method callable array.
     */
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
}
