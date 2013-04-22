<?php
namespace asm;

require('/var/www/framewire/Load.inc');

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

class asmblr extends \fw\App
{
    public $SysOp = 'asmblr@stackware.com';
    public $LogPublic = FALSE;
    public $BaseURL = '';

    // must be only a domain
    public $ConsoleDomain = 'asmblr.local';
    // the mongo db to use
    public $MongoDB = 'asmblr';

    protected $asmdb;


    public function __construct()
    {
        define('DOC_ROOT',getcwd().DIRECTORY_SEPARATOR);
        define('APP_ROOT',str_replace('DOC_ROOT','APP_ROOT',DOC_ROOT));

        \fw\Inc::Ext('Mongo.inc');
        \fw\Inc::Dir('asmblr');
    }

    public function Go()
    {
        $mongo = new \fw\Mongo;
        $this->asmdb = $mongo->Alias($this->MongoDB,'asmdb');
        $this->Wire($this->asmdb,'asmdb');

        $Domain = Request::Hostname();

        if( $this->ConsoleDomain === $Domain )
        {
            $this->BaseURL = "http://{$this->ConsoleDomain}";
            $this->CalcURLs();

            $this->Console();
        }
        else if( ($Site = $this->Match($Domain)) !== NULL )
        {
            $this->BaseURL = $Site['BaseURL'];
            $this->CalcURLs();

            $this->Srv($Site);
        }
        else
        {
            echo 'Invalid request';
            \fw\HTTP::_404();
        }
    }

    public function Console()
    {
        $this->NoName();

        $page = new \fw\KeyValueSet;
        $ps = new \fw\PageSet;
        $html = new \fw\enUSHTMLSet;
        $lp = new \fw\LinkPage($ps,$this->SiteURL);
        $ls = new \fw\LinkSet(Request::Hostname());
        $msg = new \fw\Messager;
        $vr = new \fw\ValidationReport('error');

        $this->Wire(array('page'=>$page,'ps'=>$ps,'html'=>$html,'lp'=>$lp,'ls'=>$ls,
                          'msg'=>$msg,'vr'=>$vr));

        $html->ConnectWireables($page,$lp,$ls,$msg,$vr);

        $page->Title = 'asmblr Console';
        $page->Description = 'asmblr Console';

        \fw\Inc::Dir('Routines');
        $html->LoadDir('HTML');

        if( \fw\Path::Top($this->MatchPath) === 'restv1' )
        {
            \fw\HTTP::ContentType('json');
            $as = new AccountSet($this->asmdb);
            $ss = new SiteSet($this->asmdb);
            $this->Wire(array('as'=>$as,'ss'=>$ss));

            REST::v1($this->MatchPath);
            $html->ajf_JSONResponse();
            return;
        }

        $SH = new SessionStoreMongoDB($this->asmdb);
        session_set_save_handler($SH);
        session_start();


        $ps->Create('Home','/','Request::Home');
        $ps->Create('Test','/test','Request::Test');

        $ps->Create('CSS','/css/','Request::CSSHandler');
        $ps->Create('JS','/js/','Request::JSHandler');
        $ps->Create('AjaxFrags','/ajf/','AjaxFrags');

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
        return $this->asmdb->findOne(array('Domain'=>$Domain));
    }

    // Executing a site involves:
    //  - adding config parameters to this asmblr object
    //  - applying directives (same as PageSet)
    //  - executing the routine (same as PageSet)
    //  - add site templates
    //  - match and execute a page
    //  - render
    // TODO: handle lib code
    public function Srv( $Site )
    {
        foreach( $Site['Config'] as $K => $V )
            $this->{$K} = $V;

        $this->NoName();

        $ps = new PageSet($this->asmdb);

        $html = new enUSHTMLSet($this->asmdb);
        $html->Load($Site['_id']);

        $lp = new \fw\LinkPage($ps,$this->SiteURL);
        $ls = new \fw\LinkSet($Site['Domain']);

        $this->Wire(array('ps'=>$ps,'html'=>$html,'lp'=>$lp,'ls'=>$ls));

        $html->ConnectWireables($lp,$ls);

        foreach( $Site['Directives'] as $V )
        {
            if( ($W = $this->{$V['Name']}) === NULL )
                throw new Exception("Directive object {$V['Name']}' doesn't exist while executing Site '{$Site['Domain']}'.");
            else
                $W->ApplyDirective($V['Key'],$V['Value']);
        }

        if( empty($Site['Routine']) === FALSE )
        {
            if( $Site['Routine']['Type'] === 'Pointer' )
                $Site['Routine'][0]::$Site['Routine'][1]();
            else
                eval($Site['Routine'][0]);
        }

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

    protected function NoName()
    {
        // NOTE: should $this be set global, or that from the matched site?
        $GLOBALS['FWAPP'] = $this;

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


    /*
    public function CalcURLs( $BaseURL,$Request )
    {
        $SiteURL = $Request;
        Path::Lower($SiteURL['Path']);
        $MatchPath = array();

        $IsBaseScheme = $IsBaseHostname = $IsBasePort = $IsBasePath = TRUE;

        if( empty($BaseURL) )
        {
            $MatchPath = $SiteURL['Path'];
            $MatchPath['IsDir'] = FALSE;
            $SiteURL['Path'] = Path::Init('/');
        }
        else
        {
            // sort of a hack in case we get a URL struct as a BaseURL which we often do
            // if it's a struct, we assume no asterisk replace is needed, which is
            // probably wrong to do
            if( !is_array($BaseURL) )
            {
                $BaseURL = URL::Init(str_replace('*',Hostname::ToString($SiteURL['Hostname']),$BaseURL));
            }

            if( !empty($BaseURL['Scheme']) )
            {
                $IsBaseScheme = ($BaseURL['Scheme'] === $SiteURL['Scheme']);
                URL::SetScheme($BaseURL['Scheme'],$SiteURL);
            }

            if( !empty($BaseURL['Hostname']) )
            {
                $IsBaseHostname = ($BaseURL['Hostname'] === $SiteURL['Hostname']);
                URL::SetHostname($BaseURL['Hostname'],$SiteURL);
            }

            if( !empty($BaseURL['Port']) )
            {
                $IsBasePort = ($BaseURL['Port'] === $SiteURL['Port']);
                URL::SetPort($BaseURL['Port'],$SiteURL);
            }

            $MatchPath = $SiteURL['Path'];
            $MatchPath['IsDir'] = FALSE;

            if( $BaseURL['Path']['IsRoot'] === FALSE )
            {
                // @todo More efficient way of doing this...?
                foreach( $BaseURL['Path']['Segments'] as $K => $V )
                    $IsBasePath = isset($SiteURL['Path']['Segments'][$K]) && $SiteURL['Path']['Segments'][$K] === $V;

                URL::SetPath($BaseURL['Path'],$SiteURL);
                Path::Mask($SiteURL['Path'],$MatchPath);
            }
            else
                $SiteURL['Path'] = Path::Init('/');
        }

        return array('SiteURL'=>$SiteURL,'MatchPath'=>$MatchPath,
                     'IsBaseScheme'=>$IsBaseScheme,'IsBaseHostname'=>$IsBaseHostname,
                     'IsBasePort'=>$IsBasePort,'IsBasePath'=>$IsBasePath);
    }
    */


    // TODO: fully implement/document
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


    public function NoPageHandler()
    {
        \fw\HTTP::_404();
        if( isset($this->html) )
        {
            $this->html->ReMap('Article','Error404');
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


// that's it!  we're so encapsulated
$asm = new asmblr;
$asm->Go();


define('TIME_TO_BUILD',round((microtime(TRUE)-START_TIME),4)*1000.000);

