<?php
/**
 * @file Instance.inc asmblr instance controller.
 * @author Stackware, LLC
 * @version 4.0
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * Base application loader and request router.
 *
 * It should be extended and customized in index.php.
 *
 * @note We're using only Google Drive (file system) and no mongo/etc.
 *
 * @todo Include hook for an instance-wide lib
 * @todo Consider an instance manifest.
 * @note asmblr error logging like llog()/console isn't available in these methods because they haven't been set yet (done in app)
 */
abstract class Instance
{
    protected $Apps = array();

    protected $RequestedHostname;

    // gs://asmblr-mc-tmp/
    protected $CacheDir = '';

    protected $CacheManifest = FALSE;
    protected $CacheApp = FALSE;


    public function __construct()
    {
        // seems to hold us for linux/windows/gae
        if( empty($this->CacheDir) )
            $this->CacheDir = sys_get_temp_dir();
    }

    /**
     * Match a requested hostname to an app.
     *
     * The hostname is resolved to an app manifest which then
     * loads the app itself into memory and returns the manifest.
     *
     * @param array $Request Request array to process.
     * @retval array Manifest of the routed to app.
     * @throws Exception Can't read directory for requested hostname.
     * @throws Exception Unknown requested hostname.
     * @todo Implement locking/flock() for writing cached files.
     */
    public function Match( $Request )
    {
    	$this->RequestedHostname = Hostname::ToString($Request['Hostname']);

    	// determine if we have an app for the exact hostname
    	if( isset($this->Apps[$this->RequestedHostname]) )
    	{
    	    $App = $this->Apps[$this->RequestedHostname];
    	}
    	// if not, find the most general match which must be defined with a leading period
    	else
        {
            foreach( Hostname::Order($Request['Hostname']) as $H )
            {
                if( isset($this->Apps[$H]) )
                {
                    $App = $this->Apps[$H];
                    break;
                }
            }
        }

        if( empty($App) )
            throw new Exception("Unknown requested hostname '{$this->RequestedHostname}'");

        // make sure we're not misconfigured - this should always be an absolute dir which comes from
        // our Instance::Apps config
        if( !is_dir($App[0]) )
            throw new Exception("Can't read '{$App[0]}' for requested hostname '{$this->RequestedHostname}'");

        if( $this->CacheManifest === TRUE )
        {
            if( (@include "{$this->CacheDir}/{$this->RequestedHostname}.manifest.inc") === FALSE )
            {
                $Manifest = $this->BuildAppManifest($App,$this->RequestedHostname);
                file_put_contents("{$this->CacheDir}/{$this->RequestedHostname}.manifest.inc",'<?php $Manifest = '.var_export($Manifest,TRUE).';');
            }
        }
        else
        {
            // clear an existing cache
            @unlink("{$this->CacheDir}/{$this->RequestedHostname}.manifest.inc");
            $Manifest = $this->BuildAppManifest($App,$this->RequestedHostname);
        }

        if( $this->CacheApp === TRUE )
        {
/*
            $Options = array('gs'=>array('Content-Type'=>'text/plain'));
            $ctx = stream_context_create($Options);

            $B = file_get_contents("{$this->CacheDir}/{$Hostname}.app.inc",false,$ctx);
            var_dump($B);
            //            eval($B);
            exit('ok');
            include "{$this->CacheDir}/{$Hostname}.app.inc";

            //            var_dump($B);
            //            eval($B);
            exit;
*/
            // PHP bug: always throws fatal error with no ability to detect @ when consecutive after above include()
            if( (@include "{$this->CacheDir}/{$this->RequestedHostname}.app.inc") === FALSE )
            {
                file_put_contents("{$this->CacheDir}/{$this->RequestedHostname}.app.inc",$this->BuildApp($Manifest));
                include "{$this->CacheDir}/{$this->RequestedHostname}.app.inc";
            }
        }
        else
        {
            // clear an existing cache
            @unlink("{$this->CacheDir}/{$this->RequestedHostname}.app.inc");
            $this->LoadApp($Manifest);
        }

        return $Manifest;
    }


    // use a manifest.inc if it exists otherwise build from manifest.gsheet in the $AppRoot
    // $AppRoot is trusted - note that on Windows sometimes the file permissions have to be adjusted
    // for both the directory and the manifest.gsheet file itsef
    // hostname is now determined by our instance config, not the manifest.gsheet
    protected function BuildAppManifest( $App,$Hostname )
    {
        $AppRoot = Path::Init($App[0]);
        $AppRoot['IsDir'] = TRUE;
        $AppRoot = Path::ToString($AppRoot);

        // load and process as a Google Sheet and return a manifest array
        if( !is_readable($AppRoot.'manifest.gsheet') )
            throw new Exception("Can't find manifest.gsheet in $AppRoot");

        // Pages/PageMaps are numeric arrays of one of more page sets
        $Manifest = array('AppRoot'=>$AppRoot,'DefaultPageSet'=>$App[1],'Config'=>array(),
                          'Directives'=>array(),'Pages'=>array(),'PageMaps'=>array(),'Templates'=>array());

        // pull the main URL from our local gsheet file
        $SS = file_get_contents($AppRoot.'manifest.gsheet');
        $SS = json_decode($SS,TRUE);
        $SS = $OrigURL =  URL::Init($SS['url']);

        // munge to be exportable as html
        $SS['Path']['Segments'][1] = 'pub';
        $SS['Query']['output'] = 'html';
        $SS['Query']['usp'] = NULL;

        // read in the HTML version which we scrape for each of the tabs
        $Buf = file_get_contents(URL::ToString($SS));
        $Stats = $this->ParseManifestHeaders($http_response_header);

        if( empty($Stats['HTTP']) )
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
        $Tabs = array('FIRST'=>URL::ToString($SS));

        $R = $XPR->query('//ul[@id="sheettabs"]/li/a');

        foreach( $R as $V )
        {
            // tabs must be unique named
            $Tabs[$V->textContent] = $V->getAttribute('href');
        }

        // go over each tab and pull as CSV and parse into our manifest
        foreach( $Tabs as $Label => $T )
        {
            if( strtolower($Label) === 'reference' )
                continue;

            $SS = URL::Init($T);
            // munge to get CSV
            $SS['Query']['output'] = 'csv';

            $B = @file(URL::ToString($SS));
            $Stats = $this->ParseManifestHeaders($http_response_header);

            // something isn't right so just skip (not 200 or not csv)
            if( empty($Stats['HTTP']) || empty($Stats['CSV']) )
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
            throw new Exception("Couldn't find a config tab at '".URL::ToString($OrigURL));

        // lastly, add some instance info - handy for debugging
        $Manifest['CacheManifest'] = $this->CacheManifest;
        $Manifest['CacheApp'] = $this->CacheApp;

        $Manifest['Config']['Hostname'] = $Hostname;
        $Manifest['Config']['AppRoot'] = $AppRoot;

        return $Manifest;
    }

    // just pulls out/normalizes HTTP response and content type for now
    private function ParseManifestHeaders( $Raw )
    {
        $Headers = array();
        foreach( $Raw as $V )
        {
            if( strpos($V,'HTTP/') === 0 )
            {
                if( strpos($V,'200 OK') !== FALSE )
                    $Headers['HTTP'] = TRUE;
                else
                    $Headers['HTTP'] = FALSE;
            }
            else if( strpos($V,'Content-Type') === 0 )
            {
                if( strpos($V,'text/csv') !== FALSE )
                    $Headers['CSV'] = TRUE;
                else
                    $Headers['CSV'] = FALSE;
            }
        }

        return $Headers;
    }


    // include() each file from functions/lib when in non-cache/dev mode
    // templates aren't included themselves, though the name of each file with any prefix is saved in the manifest
    protected function LoadApp( &$Manifest )
    {
        $AppRoot = $Manifest['AppRoot'];

        if( !is_dir($AppRoot) || !is_readable($AppRoot) )
            throw new Exception("Unreadable AppRoot '{$AppRoot}'");

        $Hostname = $Manifest['Config']['Hostname'];

        // PHP Bug - can't use FilesystemIterator::CURRENT_AS_PATHNAME |  FilesystemIterator::KEY_AS_FILENAME with recursive dirs
        $Flags = \FilesystemIterator::SKIP_DOTS | \FilesystemIterator::UNIX_PATHS;

        // subdirectories are NOT prefixed and each file just include()'d
        $dir = new \RecursiveDirectoryIterator("{$AppRoot}lib",$Flags);
        $fs = new \RecursiveIteratorIterator($dir);
        foreach( $fs as $K => $V )
        {
            include $K;
        }

        // subdirectories are NOT prefixed and each file just include()'d
        $dir = new \RecursiveDirectoryIterator("{$AppRoot}functions",$Flags);
        $fs = new \RecursiveIteratorIterator($dir);
        foreach( $fs as $K => $K )
        {
            include $K;
        }

        // subdirectories ARE prefixed and a Template struct is created with the proper name and filesystem path and merged with
        // any Function definition from the manifest - each is stored in the manifest - the actual template is NOT include()'d
        $dir = new \RecursiveDirectoryIterator("{$AppRoot}templates",$Flags);
        $fs = new \RecursiveIteratorIterator($dir);
        foreach( $fs as $K => $V )
        {
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

                if( !empty($Manifest['Templates'][$Prefix.$P['Segments'][1]]) )
                    $F = $Manifest['Templates'][$Prefix.$P['Segments'][1]];
                else
                    $F = array();

                $Manifest['Templates'][$Prefix.$P['Segments'][1]] = Template::Init($Prefix.$P['Segments'][1],$K,$F);
            }
            // supporting @@@ is a bit annoying but very useful for SQL and even stacking JS frags so
            // we're keeping it for now
            // note that these DO get their body populated and so are eval()'d rather than being included
            // note also that the name here will be prefixed with a sub-directory
            // this regex needs to be more robust, including handling empty frags, frags that don't
            // start with whitespace, those that end with a comment, etc.
            else
            {
                $B = preg_split("/\s*@@@(\w+[a-zA-Z0-9\-]*)/m",$Buf,0,PREG_SPLIT_DELIM_CAPTURE|PREG_SPLIT_NO_EMPTY);
                $CNT = count($B);

                for( $i = 0; $i < $CNT; $i+=2 )
                {
                    if( !empty($Manifest['Templates'][$Prefix.$B[$i]]) )
                        $F = $Manifest['Templates'][$Prefix.$B[$i]];
                    else
                        $F = array();

                    $Manifest['Templates'][$Prefix.$B[$i]] = Template::Init($Prefix.$B[$i],$K,$F,$B[$i+1]);
                }
            }
        }
    }

    protected function BuildApp( &$Manifest )
    {
        $AppRoot = $Manifest['AppRoot'];

        if( !is_dir($AppRoot) || !is_readable($AppRoot) )
        	throw new Exception("Unreadable AppRoot '{$AppRoot}'");

        $Hostname = $Manifest['Config']['Hostname'];

        $AppFile = '';

        // PHP Bug - can't use FilesystemIterator::CURRENT_AS_PATHNAME |  FilesystemIterator::KEY_AS_FILENAME with recursive dirs
        $Flags = \FilesystemIterator::SKIP_DOTS | \FilesystemIterator::UNIX_PATHS;

        // subdirectories are NOT prefixed
        $dir = new \RecursiveDirectoryIterator("{$AppRoot}lib",$Flags);
        $fs = new \RecursiveIteratorIterator($dir);
        foreach( $fs as $K => $V )
        {
            $AppFile .= "\n\n\n/*** {$K} ***/";

            $T = php_strip_whitespace($K);
            if( strpos($T,'<?php') === 0 )
                $AppFile .= "\n".substr($T,5);
            else
                $AppFile .= $T;
        }

        // subdirectories are NOT prefixed
        $dir = new \RecursiveDirectoryIterator("{$AppRoot}functions",$Flags);
        $fs = new \RecursiveIteratorIterator($dir);
        foreach( $fs as $K => $K )
        {
            $AppFile .= "\n\n\n/*** {$K} ***/";

            $T = php_strip_whitespace($K);
            if( strpos($T,'<?php') === 0 )
                $AppFile .= "\n".substr($T,5);
            else
                $AppFile .= $T;
        }

        // subdirectories ARE prefixed and merge any Function definition from the manifest
        $AppFile .= "\n\n\n/*** {$AppRoot}templates ***/";
        $dir = new \RecursiveDirectoryIterator("{$AppRoot}templates",$Flags);
        $fs = new \RecursiveIteratorIterator($dir);
        $Templates = array();
        foreach( $fs as $K => $V )
        {
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

                if( !empty($Manifest['Templates'][$Prefix.$P['Segments'][1]]) )
                    $F = $Manifest['Templates'][$Prefix.$P['Segments'][1]];
                else
                    $F = array();

                // basically adding Buf is the only difference from LoadApp() - for now :)
                $Manifest['Templates'][$Prefix.$P['Segments'][1]] = Template::Init($Prefix.$P['Segments'][1],$K,$F,$Buf);
            }
            else
            {
                $B = preg_split("/\s*@@@(\w+[a-zA-Z0-9\-]*)/m",$Buf,0,PREG_SPLIT_DELIM_CAPTURE|PREG_SPLIT_NO_EMPTY);
                $CNT = count($B);
                for( $i = 0; $i < $CNT; $i+=2 )
                {
                    if( !empty($Manifest['Templates'][$Prefix.$B[$i]]) )
                        $F = $Manifest['Templates'][$Prefix.$B[$i]];
                    else
                        $F = array();

                    $Manifest['Templates'][$Prefix.$B[$i]] = Template::Init($Prefix.$B[$i],$K,$F,$B[$i+1]);
                }
            }
        }

        return "<?php\n\n{$AppFile} \n\n \$Manifest['Templates'] = ".var_export($Manifest['Templates'],TRUE).';';
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
