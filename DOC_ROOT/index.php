<?php
/**
 * @file asmsrv.php asmblr boot file.
 * @author Stackware, LLC
 * @version 4.0
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */

// Optional
define('START_TIME',microtime(TRUE));


/**
 * The absolute path to asmblr's loader.
 *
 * CONFIG: You should use an absolute path in production.
 *
 * Recommended paths for asmblr:
 *  - c:/inetpub/asmblr/core/Load.inc
 *  - /var/www/asmblr/core/Load.inc
 */
require('../core/Load.inc');


/**
 * Instance specific configuration.
 *
 * Remember, these are instance-wide configuration settings that will be applied
 * to ALL apps running under the instance.
 */
class Instance extends \asm\Instance
{
    /**
     * Define known hostnames and their apps.
     *
     * Must be of the form hostname => absolute directory  (always use forward slashes)
     *
     * The hostname will be used as the base hostname which can be overridden by BaseURL
     *
     * Exact hostname matching is performed first, then ordered matching is performed,
     * least specific to most specific, which match domains prefixed with a period.
     */
    protected $Apps = array('mc2.stackop.com'=>'/var/www/asmblr-apps/MC',
                            'mmc.stackop.com'=>'/var/www/asmblr-apps/MC');

    /**
     * Set the local cache directory.
     *
     * It must be outside of any document root or Google Drive share, already
     * exist, and be writeable by the web server (typically nobody or apache).
     *
     * Always use forward slashes.
     *
     * @todo Can this be detected automatically on Windows, and default to something
     *       reasonable on Linux so that it doesn't have always be explicitly set?
     */
    protected $CacheDir = '/tmp/';

    /**
     * Set to TRUE to cache app manifests from Google Docs.
     * Setting to FALSE will disable and delete an existing cache file.
     */
    protected $CacheManifest = FALSE;

    /**
     * Set to TRUE to cache apps built from disk.
     * Setting to FALSE will disable and delete an existing cache file.
     */
    protected $CacheApp = FALSE;
}


/**
 * Application specific configuration and execution.
 */
class App extends \asm\App
{
    // for full control of execution, extend/override this method
    public function __construct( $Request,$Manifest )
    {
        parent::__construct($Request,$Manifest);

        // start with applying general purpose config settings
        $C = $this->Config;

        if( empty($C['Status']) || $C['Status'] !== 'Active' )
            HTTP::_400();

        // set a base directory
        if( !empty($C['open_basedir']) )
            $this->OpenBaseDir($C['open_basedir']);

        // Activate the Framewire error handler
        if( empty($C['error_handler']) )
            set_error_handler(array($this,'ErrorHandler'));
        else
            set_error_handler(array($this,$C['error_handler']));

        // Calculate app-wide URLs, modifying our Request array.  This sets
        // $SiteURL and $MatchPath which are used for creating URLs (LinkSet) and
        // matching pages (PageSet), respectively.
        Request::CalcURLs($this->Request,$C['BaseURL']);

        // instantiate system objects
        // this sets dynamic properties - know your names
        $this->ps = new \asm\PageSet($this);

        $this->html = new \asm\enUSHTMLSet($this);

        // links for pages (internal linking)
        $this->lp = new \asm\LinkPage($this->ps,$this->Request['SiteURL']);

        // links for managed images (via cnvyr)
        $this->limg = new \asm\Linkcnvyr($this->Config['Hostname'].'/cnvyr/');

        // general purpose key/value store
        $this->page = new \asm\KeyValueSet;

        // session based UI messages/alerts
        $this->msg = new \asm\Messager;

        // track form/request validation using an 'has-error' CSS class
        $this->vr = new \asm\ValidationReport('has-error');

        // templates should have access to some of these so Connect() them in
        // if you later Connect() something using the same name it will overwrite the object here
        $this->html->Connect(array('lp'=>$this->lp,'limg'=>$this->limg,'page'=>$this->page,'msg'=>$this->msg,'vr'=>$this->vr));


        // uncomment to demonstrate mongo connectivity
        /*
        \asm\Inc::Ext('Mongo.inc');
        $mongo = new \asm\Mongo;
        $this->mydb = $mongo->Alias('mydb','mydb');
        */


        // if we're not caching the manifest we know we're not in production, so implement
        // some easy toggling of debugging pages and templates and tweak our links to persist
        if( empty($Manifest['CacheManifest']) )
        {
            if( !empty($_GET['debug']) )
            {
                $this->lp->SetBaseURL($this->Request['SiteURL'],$_GET);

                $this->ps->DebugOn();
                $this->html->DebugOn();

                if( isset($this->mydb) )
                    $this->mydb->DebugOn();
            }
        }


        // match pages against the MatchPath to determine our executing page
        // this logic is left verbose for easy customization

        $OrderedMatch = $ExactMatch = NULL;

        // first attempt a hierarchal ordered match/execute - most general to most specific URL path
        if( $this->Request['MatchPath']['IsRoot'] === FALSE )
        {
            foreach( \asm\Path::Order($this->Request['MatchPath']) as $V )
            {
                if( ($OrderedMatch = $this->ps->Match($V)) !== NULL )
                    break;
            }
        }

        // if an ordered match isn't found, attempt an exact match
        // this and other matching behavior can be easily customized
        // NOTE: This means that if a page with /admin/ matches, a page with /admin/something WILL NOT
        // be executed
        if( $OrderedMatch === NULL )
            $ExactMatch = $this->ps->Match(\asm\Path::ToString($this->Request['MatchPath']));

        // determine which page to execute
        if( $OrderedMatch !== NULL )
            $ExecPage = $OrderedMatch;
        else if( $ExactMatch !== NULL )
            $ExecPage = $ExactMatch;
        else
            $ExecPage = NULL;

        if( empty($ExecPage) || $ExecPage['Status'] !== 'Active' )
            $this->NoPageHandler();

        // asmblr supports CLI execution natively, though some config directives
        // only make sense when serving an HTTP request, or for certain pages
        if( $this->Request['IsCLI'] === FALSE )
        {
            // IsBaseHostname indicates whether the request is using the full hostname
            // configured by BaseURL.  If not, redirect to it.
            if( $C['ForceBaseHostname'] === TRUE && $this->Request['IsBaseHostname'] === FALSE )
            {
                $this->Request['Hostname'] = $this->Request['BaseURL']['Hostname'];
                HTTP::Location($this->Request);
            }

            // start a session if appropriate
            if( !empty($C['StartSession']) && (strpos($C['PageNoSession'],$ExecPage['Name']) === FALSE))
                session_start();

            // seems to cause encoding errors, like for IE when the request doesn't finish normally
            // and sometimes with Apache, though rarely - only when we var_dump early in execution it seems
//             if( !empty($C['zlib_output']) )
//                 ini_set('zlib.output_compression',TRUE);

            if( !empty($C['mb_http_output']) )
                mb_http_output($C['mb_http_output']);

            // these can also be handled when creating the enUSHTMLSet template object
            if( empty($C['ContentType']) )
                $C['ContentType'] = 'text/html; charset=UTF-8';

            header("Content-Type: {$C['ContentType']}");
        }

        // we're ready to execute the page and honor any app-wide directives or function
        // note that this could be reordered if needed
        foreach( $this->Directives as $V )
        {
            if( empty($this->{$V[0]}) )
                throw new Exception("Directive object {$V[0]}' doesn't exist while executing app for '{$this->Config['Hostname']}'.");
            else
                $this->{$V[0]}->ApplyDirective($V[1],$V[2]);
        }

        // optionally execute a function just prior to the page and rendering
        // if this function returns FALSE, nothing further is done
        if( !empty($C['PreFunction']) )
        {
            if( $C['PreFunction']($this) !== FALSE )
            {
                // now execute the actual page
                $this->ps->Execute($ExecPage);

                // and finally begin rendering at the Base.tpl template
                $this->html->Base();
            }
        }
        // otherwise just execute the page and render
        else
        {
            // now execute the actual page
            $this->ps->Execute($ExecPage);

            // and finally begin rendering at the Base.tpl template
            $this->html->Base();
        }
    }


    /**
     * Handle a Page not found (404) error.
     *
     * Execution stops at this function.
     */
    public function NoPageHandler()
    {
        // Send the appropriate HTTP header.
        \asm\HTTP::_404();

        // Ensure we have a $html around and swap in the 404 template.
        if( isset($this->html) )
        {
            $this->html->ReMap('Layout','Error404');

            // Start rendering here since we don't return from this method.
            $this->html->Base();
        }

        // Kill execution.
        exit;
    }
}


try
{
    $Request = \asm\Request::Init();

    $inst = new Instance;

    $Manifest = $inst->Match($Request);

    // this does it all though could be multiple functions
    $app = new App($Request,$Manifest);
}
catch( \Exception $E )
{
    if( isset($app) )
        $app->html->Error500();

    trigger_error($E->getMessage());
    trigger_error($E->getTraceAsString());

    \asm\HTTP::_500();
}

