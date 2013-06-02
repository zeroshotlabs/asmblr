<?php
namespace asm;

require('../../framewire/Load.inc');

/**
 * asmblr has three main parts:
 *   - Console: standard Framewire app front-end for managing sites (REST + direct)
 *       $ConsoleURL:  https://asm1.stackop.com/
 *
 *   - REST API: specialized Framewire app for manipulating hosted sites
 *       $RESTURL:  https://asm1.stackop.com/restv1/
 *
 *   - Asmblr Server: specialized asmblr app CMS and cloud site hosting
 *       not configurable, i.e. http://any.thing.com/
 *       or automatically available via http://anythingcom.asm1.stackop.com/
 *
 * And so LiveEdit?!?!?
 *
 * Do we want a asm() function to replace fw()  ?
 *
 * Asmblr Startup

	1. Connect Mongo + sessions
	2. If request hostname matches ConsoleURL hostname
		a. Instantiate Console app object, Go()
			i. CalcURLs
			ii. Create FW \fw\PageSet, \fw\TemplateSet, \fw\LinkSet, etc.
			iii. Wire
			iv. If bottom-domain matches domain, live-edit/serving magic???
				1) Create/connect \asm\PageSet, \asm\TemplateSet
			v. If Match() REST URL
				1) Handoff to REST controller routine
					a) Create REST handling routines
					b) Route and execute
			vi. Otherwise a standard FW app as console
				1) NoName
				2) OrderedMatch/Match for console UI and execute
				3) Render
	3. Otherwise we have to serve an asmblr site
		a. If Asmblr::Match(Domain)
			i. CalcURLs
			ii. Create/connect \asm\PageSet, asm\TemplateSet
			iii. Wire
			iv. Execute
		b. Site not found/invalid request (same as http://www.srvr.co/)
 */

// asmblr multi-site server
class asmSrv extends \fw\App
{
    protected $asmdb;
    protected $SrvSite;

    public $SysOp = 'asmblr@stackware.com';
    // will need some way to reliably control this
    public $LogPublic = TRUE;
    public $MongoDB = 'asmblr';

    public $SiteStatuses = array('Active','Disabled');
    public $DirWireables = array('page','html');


    public function __construct()
    {
        // use asm()
        $GLOBALS['ASMAPP'] = $this;

        // hack - framewire complains during srv, but console then conflicts
        if( empty($GLOBALS['FWAPP']) )
            $GLOBALS['FWAPP'] = $GLOBALS['ASMAPP'];

        $mongo = new \fw\Mongo;
        $this->asmdb = $mongo->Alias($this->MongoDB,'asmdb');

        // wire up the basics that we need
        $as = new AccountSet($this->asmdb);
        $ss = new SiteSet($this->asmdb);
        $this->Wire(array('as'=>$as,'ss'=>$ss,'asmdb'=>$this->asmdb));

        // now do more site specific stuff
        // TODO: these may become configurable in Site['Config']
        set_error_handler(array($this,'ErrorHandler'));
        set_exception_handler(array($this,'UncaughtExceptionHandler'));
        register_shutdown_function(array($this,'FatalErrorHandler'));

        // TODO: optimize via .ini or make configurable and consider with locale Templates (enUSHTMLSet).
        // problematic with media serving
        \fw\HTTP::ContentType('text/html','utf-8');
        mb_http_output('UTF-8');
        ini_set('zlib.output_compression',TRUE);
    }

    // this always completes if the site is found, regardless of it's status
    public function GetSet( $Domain,$Request )
    {
        if( ($this->SrvSite = $this->Match($Domain)) === NULL )
            return FALSE;

        $this->BaseURL = $this->SrvSite['BaseURL'];
        $this->Request = $Request;

        list($this->SiteURL,$this->MatchPath,$this->IsBaseScheme,
             $this->IsBaseHostname,$this->IsBasePort,$this->IsBasePath) = $this->CalcURLs($this->Request,$this->BaseURL);

        foreach( $this->SrvSite['Config'] as $K => $V )
            $this->{$K} = $V;

        // and now our typical application level stuff
        // this will need review/etc per a site's runtime, REST::util_dir_names() and DirectiveNames in Console::Site()
        $page = new \fw\KeyValueSet;
        // probably should be protected better, but handy for now
        $page->ActiveSite = $this->SrvSite;

        // have to manually set collection to TemplateSet
        $html = new enUSHTMLSet($this->asmdb,$this->SrvSite['_id'],'TemplateSet');

        $ps = new PageSet($this->asmdb,$this->SrvSite['_id']);

        $lp = new \asm\LinkPage($ps,$this->SiteURL);
        $ls = new \fw\LinkSet($this->SrvSite['Domain']);

        $this->Wire(array('page'=>$page,'html'=>$html,'ps'=>$ps,'lp'=>$lp,'ls'=>$ls));

        // double wire - have to straighten out our naming between srv and console
//        $this->Wire(array('html'=>$html,'ts'=>$html,'ps'=>$ps,'lp'=>$lp,'ls'=>$ls));
        $html->ConnectWireables($lp,$ls,$page);
    }

    // Executing a site involves:
    //  - adding config parameters to this asmblr object
    //  - applying directives (same as PageSet)
    //  - executing the routine (same as PageSet)
    //  - add site templates
    //  - match and execute a page
    //  - render
    // TODO: handle lib code
    // serve the site - will 400 if status isn't active
    // if a page is not active, it'll throw a 400 but other pages may have executed, including the site
    // routine, directives, etc.
    public function Go()
    {
        if( $this->SrvSite['Status'] !== 'Active' )
            \fw\HTTP::_400();

        // we're doing this all over the place - need to wire/centralize somehow - or optimize even
        // since we're doing a lot of extra connections/queries/etc it seems
        // also applying directives is happening here for now, though it may be handy to have it
        // happen in GetSet() - or callable as we need it to be
        $ds = new DataSet($this->asmdb,$this->SrvSite['_id'],'Directive');
        foreach( $ds as $V )
        {
            if( ($W = $this->{$V['Name']}) === NULL )
                throw new Exception("Directive object {$V['Name']}' doesn't exist while executing Site '{$this->SrvSite['Domain']}'.");
            else
                $W->ApplyDirective($V['Key'],$V['Value']);
        }

        if( empty($this->SrvSite['Routine']) === FALSE )
        {
            if( $this->SrvSite['Routine']['Type'] === 'Pointer' )
            {
                $S = $this->SrvSite;
                $S['Routine'][0]::$S['Routine'][1]();
            }
            else
                eval($this->SrvSite['Routine'][0]);
        }

        $OrderedMatch = NULL;
        if( $this->MatchPath['IsRoot'] === FALSE )
        {
            foreach( \fw\Path::Order($this->MatchPath) as $V )
            {
                if( ($OrderedMatch = $this->ps->Match($this->SrvSite['_id'],$V)) !== NULL )
                {
                    if( $OrderedMatch['Status'] !== 'Active' )
                        \fw\HTTP::_400();

                    // hack and inefficient
                    $ds = new DataSet($this->asmdb,$this->SrvSite['_id'],'DirectiveP_'.$OrderedMatch['_id']);
                    $OrderedMatch['Directives'] = $ds;

                    $this->ps->Execute($OrderedMatch);
                    break;
                }
            }
        }

        if( ($ExactMatch = $this->ps->Match($this->SrvSite['_id'],\fw\Path::ToString($this->MatchPath))) !== NULL )
        {
            if( $ExactMatch['Status'] !== 'Active' )
                \fw\HTTP::_400();

            // hack and inefficient
            $ds = new DataSet($this->asmdb,$this->SrvSite['_id'],'DirectiveP_'.$ExactMatch['_id']);
            $ExactMatch['Directives'] = $ds;

            $this->ps->Execute($ExactMatch);
        }

        if( $ExactMatch === NULL && $OrderedMatch === NULL )
            $this->NoPageHandler();

        $this->html->Base();
    }

    /**
     * Lookup a Site by it's Domain.
     *
     * Only one or none Sites will match.
     *
     * @param string $Domain The domain/hostname to exact match.
     * @retval array The Site Struct.
     */
    public function Match( $Domain )
    {
        return $this->asmdb->SiteSet->findOne(array('Domain'=>$Domain));
    }

    // TODO: fully implement/document/use?
    public function OpenbaseDir( $Path = '' )
    {
        if( empty($Path) )
        {
            if( static::IsWindows() )
                ini_set('open_basedir',DOC_ROOT.'../;'.FW_ROOT.';C:/Windows/Temp/');
            else
                ini_set('open_basedir',DOC_ROOT.'../:'.FW_ROOT.':/tmp:/var/www/lib');
        }
        else
            ini_set('open_basedir',$Path);
    }
}



// asmblr console - standard FW app
class fwApp extends \fw\App
{
    public $SysOp = 'asmblr@stackware.com';
    public $LogPublic = TRUE;

    public static $ConsoleDomain = 'asmblr.local';


    // minimize internal startup
    public function __construct()
    {
        $GLOBALS['FWAPP'] = $this;
        set_error_handler(array($this,'ErrorHandler'));
        set_exception_handler(array($this,'UncaughtExceptionHandler'));
        register_shutdown_function(array($this,'FatalErrorHandler'));

        \fw\HTTP::ContentType('text/html','utf-8');
        ini_set('zlib.output_compression',TRUE);
    }

    public function Go()
    {
        // local FW application - nothing to do with asmblr
        \fw\Inc::Dir('Routines');

        $this->Request = \fw\Request::Init();
        $this->BaseURL = 'http://'.static::$ConsoleDomain;

        list($this->SiteURL,$this->MatchPath,$this->IsBaseScheme,
             $this->IsBaseHostname,$this->IsBasePort,$this->IsBasePath) = $this->CalcURLs($this->Request,$this->BaseURL);

        $page = new \fw\KeyValueSet;
        $ps = new \fw\PageSet;
        $html = new \fw\enUSHTMLSet;

        $lp = new \fw\LinkPage($ps,$this->SiteURL);
        $ls = new \fw\LinkSet(static::$ConsoleDomain);
        $lr = new LinkREST($this->BaseURL.'/restv1');

        $msg = new \fw\Messager;
        $vr = new \fw\ValidationReport('error');

        $this->Wire(array('page'=>$page,'ps'=>$ps,'html'=>$html,'lp'=>$lp,'ls'=>$ls,'lr'=>$lr,
                          'msg'=>$msg,'vr'=>$vr));

        $html->ConnectWireables($page,$lp,$ls,$lr,$msg,$vr);

        $page->Title = 'asmblr Console';
        $page->Description = 'asmblr Console';

        $html->LoadDir('HTML');

        // bring our asmblr stuff online - no site execution happens though (Go())
        // GetSet() will be called in our various console routines, like Page, Template
        // to setup site specific sets for Page/Template - these sets will be in Site_id mode
        $asm = new asmSrv;

        // create generic sets for helpers - these are in standalone mode
        // these are used by the API which also ties us together
        $asmps = new \asm\PageSet(asm()->asmdb);
        $asmts = new \asm\TemplateSet(asm()->asmdb);
        $this->Wire(array('asmps'=>$asmps,'asmts'=>$asmts));

        // no session created in account_auth which ties us back to this code base
        // this could probably go higher up, before $page, etc. though we use the
        // standalone sets created above
        if( \fw\Path::Top($this->MatchPath) === 'restv1' )
        {
            \fw\HTTP::ContentType('json');
            REST::v1($this->MatchPath);
            $html->JSONResponse();
            return;
        }

        // we use the asmblr session though we shouldn't probably
        $SH = new SessionStoreMongoDB(asm()->asmdb);
        session_set_save_handler($SH);
        session_start();

        if( !empty($_SESSION['LoggedIn']) && $_SESSION['LoggedIn'] === TRUE )
            $page->LoggedIn = TRUE;
        else
            $page->LoggedIn = FALSE;

        $html->RightAside = NULL;

        // helps us form the breadcrumb - either Site, Page or Template
        $page->ActiveNav = NULL;

        $ps->Create('CSS','/css/','Console::CSSHandler');
        $ps->Create('JS','/js/','Console::JSHandler');
        $ps->Create('ajfHandler','/ajf/','Console::ajfHandler');

        $ps->Create('Home','/','Console::Home');
        $ps->Create('Logout','/logout','Console::Logout');
        $ps->Create('Site','/site/','Console::Site',array('html,Article,Site'));
        $ps->Create('Page','/page/','Console::Page',array('html,Article,Page'));
        $ps->Create('Template','/template/','Console::Template',array('html,Article,Template'));

        $OrderedMatch = NULL;
        if( $this->MatchPath['IsRoot'] === FALSE )
        {
            foreach( \fw\Path::Order($this->MatchPath) as $V )
            {
                if( ($OrderedMatch = $ps->Match($V)) !== NULL )
                {
                    $ps->Execute($OrderedMatch);
                    break;
                }
            }
        }

        if( ($ExactMatch = $ps->Match(\fw\Path::ToString($this->MatchPath))) !== NULL )
            $ps->Execute($ExactMatch);

        if( $ExactMatch === NULL && $OrderedMatch === NULL )
            $this->NoPageHandler();

        $html->Base();
    }

    public function NoPageHandler()
    {
        \fw\HTTP::_404();
        if( isset($this->html) )
        {
            $this->html->ReMap('Article','Error404');
            $this->html->ReMap('LeftNav',NULL);
            $this->html->Base();
        }
        exit;
    }

    public function UncaughtExceptionHandler( \Exception $E = NULL )
    {
        $LastOutput = ob_get_clean();
        ob_start();

        // Optionally email the error.
        // \fw\Log::Email((string)$E,'ERROR');

        if( $this->html !== NULL && isset($this->html->Error500) )
        {
            $this->html->ReMap('Base','Error500');
            $this->html->Base();
        }
        else
            echo 'Critical Error.';

        $Buf = array('Exception'=>(string)$E,'LastOutput'=>$LastOutput,'$_SERVER'=>$_SERVER);
        llog($Buf,'ERROR',$E->getTrace());
        \fw\HTTP::_500();
    }
}


// Config :)
// must only be a domain
//$ConsoleHostname = 'asmblr.local';

define('DOC_ROOT',getcwd().DIRECTORY_SEPARATOR);
define('APP_ROOT',str_replace('DOC_ROOT','APP_ROOT',DOC_ROOT));

\fw\Inc::Ext('Mongo.inc');
\fw\Inc::Dir('asmblr');

// some custom start-up and route based on domain
$Domain = \fw\Request::Hostname();

if( fwApp::$ConsoleDomain === $Domain )
{
    $fw = new fwApp;
    $fw->Go();
}
else
{
    $asm = new asmSrv;
    // an unknown domain
    if( $asm->GetSet($Domain,Request::Init()) === FALSE )
        \fw\HTTP::_400();
    else
        $asm->Go();
}


define('TIME_TO_BUILD',round((microtime(TRUE)-START_TIME),4)*1000.000);

