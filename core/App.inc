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

    // the closest matched page (either from Ordered or Exact match)
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
     * @todo Do we want to create a DataSheets property?
     * @note If $App contains a Dirs element, it's used by BuildFile rather than the default directory structure.
     */
    public function __construct( $App,$Request,$ConfigOverride = array() )
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

        // we allow config variables to be overriden by the local DOC_ROOT/index.php
        // if a parameter exists in Config and in the $App, we use what's in $App
        // this means that a variable has to be defined (even if empty) in the manifest
        // note too that these results aren't cached (this whole thing could be considered "slow")
        // so whatever index.php contains at execution time, is what's used
        foreach( array_intersect_key($App,$this->Manifest['Config']) as $K => $V )
            $this->Manifest['Config'][$K] = $V;

        // finally apply hard-overrides or our own config variables - sort of hacky but
        // we need a way to set internally what we want regardless of the manifest for
        // asmblrbp-micro/etc
        foreach( $ConfigOverride as $K => $V )
            $this->Manifest['Config'][$K] = $V;

        // set local properties for the manifest - would like to streamline as it's wasteful
        // but we'd need to refactor how we pull from the cache
        $this->Config = $this->Manifest['Config'];
        $this->Pages = $this->Manifest['Pages'];
        $this->PageMaps = $this->Manifest['PageMaps'];

        // build/cache our app file
        // include/eval of this will set our $this->Templates property
        // TODO: we should test that this works without issue

        if( !empty($App['Dirs']) )
            $Dirs = $App['Dirs'];
        else
            $Dirs = array();

        if( $this->CacheApp === TRUE )
        {
            if( (@include "{$this->CacheDir}/{$this->Hostname}.app.inc") === FALSE )
            {
                file_put_contents("{$this->CacheDir}/{$this->Hostname}.app.inc",$this->BuildFile($Dirs));
                include "{$this->CacheDir}/{$this->Hostname}.app.inc";
            }
        }
        else
        {
            // clear an existing cache
            @unlink("{$this->CacheDir}/{$this->Hostname}.app.inc");
            $T = $this->BuildFile($Dirs);
            eval('?>'.$T);
        }

        if( empty($this->Config['Status']) || $this->Config['Status'] !== 'Active' )
            HTTP::_400();

        // activate asmblr's stock error handler - method can be customized in extending class
        set_error_handler(array($this,'ErrorHandler'));

        // Calculate app-wide URLs, modifying our Request array.  This creates
        // $SiteURL and $MatchPath elements which are used for creating URLs (LinkSet) and
        // matching pages (PageSet), respectively.
        Request::CalcURLs($this->Request,$this->Config['BaseURL']);


        // instantiate core system objects
        // this sets dynamic properties - know your names

        // one or more page sets are supported though there is always a single routing page set
        // as defined by $this->RoutingPS
        // the routing page set and it's linker are always available as ps and lp, respectively
        // other pagesets/linkers can be instantiated manually if needed
        // note that we use isset because an empty routing PS is ok, as long as it's been determined
        if( empty($this->RoutingPS) || !isset($this->Pages[$this->RoutingPS]) )
            throw new Exception("Invalid routing page set for '{$this->Hostname}'");

        $this->ps = new PageSet($this->Pages[$this->RoutingPS],$this->PageMaps[$this->RoutingPS]);
        $this->lp = new LinkPage($this->ps,$this,$this->Request['SiteURL']);
        // links for cnvyr managed resources (images, css, js, fonts/etc)
        $this->lc = new Linkcnvyr($this->ps,$this,$this->Request['SiteURL']);
    }

    /**
     * Match pages and execute the application.
     *
     * @note If an ordered match is found and doesn't have a Status of Weak, no exact
     * match will be attempted.
     * @note If SiteWideFunction returns FALSE, default page execution and rendering will not happen,
     * including 404 and Page status checks.
     */
    public function Execute()
    {
        // if not running as a CLI, first honor our ForceBaseHostname setting
        // IsBaseHostname indicates whether the request is using the full hostname
        // configured by BaseURL.  If not, redirect to it.
        if( $this->Request['IsCLI'] === FALSE && ($this->Config['ForceBaseHostname'] === TRUE && $this->Request['IsBaseHostname'] === FALSE) )
        {
            $this->Request['Hostname'] = $this->Request['BaseURL']['Hostname'];
            HTTP::Location($this->Request);
        }


        // match pages against the MatchPath to determine our executing page(s)
        $this->OrderedMatch = $this->ExactMatch = NULL;

        // first determine hierarchal ordered match/execute - most general to most specific URL path
        // only one page will match
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

        // if an ordered match isn't found, determine an exact match.  only one page would match
        // if the ordered match is Weak, an exact match is also allowed
        // i.e.  default:  /admin/ matches then a page with /admin/something will NOT match
        //          Weak:  /admin/ matches then a page with /admin/something WILL match
        if( $this->OrderedMatch === NULL || (strpos($this->OrderedMatch['Status'],'Weak') !== FALSE) )
        {
            $this->ExactMatch = $this->ps->Match(\asm\Path::ToString($this->Request['MatchPath']));
            $this->ClosestMatchName = $this->ExactMatch['Name'];
        }

        // apply any app-wide directives
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
     * Generic debug toggler for pages and templates.
     *
     * @todo Needs review and documented where this is called... below logic may also be incorrect.
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
     * Build the application's manifest from a Google Spreadsheet.
     *
     * A manifest URL typically looks something like:
     *   https://docs.google.com/spreadsheet/ccc?key=SOME_STRING_OF_CHARS
     *
     * @param string $ManifestURL URL to load Google spreadsheet from.
     * @retval array The application's manifest.
     */
    protected function BuildManifest( $ManifestURL )
    {
        $AppRoot = Path::Init($this->AppRoot);
        $AppRoot['IsDir'] = TRUE;
        $AppRoot = Path::ToString($AppRoot);

        $ManifestURL = $ManifestURLOrig = URL::Init($ManifestURL);
        $ManifestURLOrigStr = URL::ToString($ManifestURL);

        // new google spreadsheets
        if( strpos($ManifestURLOrigStr,'/d/') !== FALSE )
            $Manifest = $this->Manifestd($ManifestURL,$ManifestURLOrig,$AppRoot);
        // old google spreadsheets
        else
            $Manifest = $this->Manifestccc($ManifestURL,$ManifestURLOrig,$AppRoot);

        return $Manifest;
    }

    // new spreadsheets - supports data sheets
    protected function Manifestd( $ManifestURL,$ManifestURLOrig,$AppRoot )
    {
        // read in the HTML version which we scrape for each of the tabs
        $Buf = file_get_contents(URL::ToString($ManifestURL));

        $Headers = \asm\restr::ParseHeaders($http_response_header);

        if( empty($Headers['http']) || strpos($Headers['http'],'200') === FALSE )
            throw new Exception("manifest doesn't appear published in $AppRoot");

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
//            $TabCSVd = fopen('php://temp','rw');

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

//                fputcsv($TabCSVd,$Line);
            }

//            rewind($TabCSVd);
//            $Tabs[$Name] = stream_get_contents($TabCSVd);
//            fclose($TabCSVd);

            $Tabs[$Name] = $Tab;
        }

        // Pages/PageMaps are numeric arrays of one of more page sets
        // Directives are app-wide directives executed for every request.
        // DataSheets are an associative array - by sheet name - of general purpose data (key/value)
        $Manifest = array('AppRoot'=>$AppRoot,'Config'=>array(),'Directives'=>array(),
                            'Pages'=>array(),'PageMaps'=>array(),'Templates'=>array(),'DataSheets'=>array());

        // now actually build the manifest from the CSV tabs - much copied from Manifestccc
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
            throw new Exception("Couldn't find a config tab at '".URL::ToString($ManifestURLOrig));

        return $Manifest;
    }

    // old spreadsheets - does not support data sheets
    protected function Manifestccc( $ManifestURL,$ManifestURLOrig,$AppRoot )
    {
        // munge to be exportable as html
        $ManifestURL['Path']['Segments'][1] = 'pub';
        $ManifestURL['Query']['output'] = 'html';
        $ManifestURL['Query']['usp'] = NULL;

        // read in the HTML version which we scrape for each of the tabs
        $Buf = file_get_contents(URL::ToString($ManifestURL));

        $Headers = \asm\restr::ParseHeaders($http_response_header);

        if( empty($Headers['http']) || strpos($Headers['http'],'200') === FALSE )
            throw new Exception("manifest doesn't appear published in $AppRoot");

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

        // Pages/PageMaps are numeric arrays of one of more page sets
        // Directives are app-wide directives executed for every request.
        $Manifest = array('AppRoot'=>$AppRoot,'Config'=>array(),'Directives'=>array(),
                            'Pages'=>array(),'PageMaps'=>array(),'Templates'=>array());

        // go over each tab and pull as CSV and parse into our manifest
        foreach( $Tabs as $Label => $T )
        {
            // skip documentation tab
            if( strtolower($Label) === 'reference' )
                continue;

            $ManifestURL = URL::Init($T);
            // munge to get CSV
            $ManifestURL['Query']['output'] = 'csv';

            // nasty clean-up since google likes to play games
            $Tmp = @file(URL::ToString($ManifestURL));
            $B = array();
            foreach( $Tmp as $K => $V )
            {
                $Tmp2 = str_getcsv($V);
                foreach( $Tmp2 as $K2 => $V2 )
                    $Tmp2[$K2] = "\"{$V2}\"";

                $B[$K] = implode(',',$Tmp2);
            }

            $Headers = \asm\restr::ParseHeaders($http_response_header);

            // something isn't right so just skip (not 200 or not csv)
            if( empty($Headers['http']) || strpos($Headers['http'],'200') === FALSE
            || empty($Headers['content-type']) || strpos($Headers['content-type'],'text/csv') === FALSE )
                continue;

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
            else
            {
                trigger_error("Skipping unknown tab structure with name '{$Label}'");
            }
        }

        // if no config something isn't right
        if( empty($Manifest['Config']) )
            throw new Exception("Couldn't find a config tab at '".URL::ToString($ManifestURLOrig));

        return $Manifest;
    }

    // $B is an array of CSV lines
    protected function ParseTabCSV( $B )
    {
        // not needed since we put the double quotes in the strpos()'s below
        // $Header = str_getcsv($B[0]);

        // config tab - Key, Value
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
                if( empty($L[0]) )
                    continue;

                // a directive in the config tab is recognized by being three columns
                if( count($L) === 3 )
                {
                    $Tab['Directives'][] = array(str_replace('$','',$L[0]),$L[1],$L[2]);
                }
                // otherwise taken as a key/value config parameter and trim/normalize
                else
                {
                    if( $L[0] === 'SitewideFunction' )
                        $Tab['Config']['SitewideFunction'] = $this->ParseFunctionName($L[1]);
                    else
                    {
                        $Tab['Config'][$L[0]] = '';

                        // trim and normalize to boolean/NULL if a value was set.
                        if( isset($L[1]) )
                        {
                            $Tab['Config'][$L[0]] = trim($L[1]);

                            $T = strtolower($Tab['Config'][$L[0]]);

                            if( $T === 'false' )
                                $Tab['Config'][$L[0]] = FALSE;
                            else if( $T === 'true' )
                                $Tab['Config'][$L[0]] = TRUE;
                            else if( $T === 'null' )
                                $Tab['Config'][$L[0]] = NULL;
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

                    if( !empty($LastPage) )
                        $Page[$LastPage]['Directives'][] = array(str_replace('$','',$D[0]),$D[1],$D[2]);
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

                        $Tab['DataSheet'][$ID][$V] = $L[$I];
                    }
                }
            }

            return $Tab;
        }

        return array();
    }


    // if CacheApp isn't TRUE, we leave templates on disk and will include them - this helps with debugging
    // since PHP's error messages directly point to a line and file - see also Template::__call()
    // only .php and .inc files are pulled in - .tpl and .html and .js and .css for templates
    // if $Dirs is non-empty array, it's taken as array('lib'=>array(),'functions'=>array(),'templates'=>array())
    // and each directory is loaded appropriately - otherwise, the default lib/functions/templates dirs are loaded
    // directories specified must exist - they will not be gracefully skipped
    protected function BuildFile( $Dirs = array() )
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
