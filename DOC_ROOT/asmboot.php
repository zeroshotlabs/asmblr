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
 *       not configurable, i.e.
 *       http://any.thing.com/
 *       http://anythingcom.asm1.stackop.com/
 *
 * And so LiveEdit?!?!?
 */

// asmblr console, REST, and multi-site server
class asmblr extends \fw\App
{
    public $SysOp = 'asmblr@stackware.com';
    public $LogPublic = FALSE;
    public $BaseURL = '';

    public $ConsoleURL = 'http://asmblr.local/';
    public $RESTURL = 'https://asmblr.local/restv1/';


    public function __construct()
    {
        $this->ConsoleURL = URL::Init($this->ConsoleURL);
        $this->RESTURL = URL::Init($this->RESTURL);
    }

    public function GoConsole()
    {

        // general framewire application startup using our Console URL
        $this->NoName();
        $this->BaseURL = URL::ToString($this->ConsoleURL);
        $this->CalcURLs();

        $page = new \fw\KeyValueSet;
        $ps = new \fw\PageSet;
        $html = new \fw\enUSHTMLSet;
        $lp = new \fw\LinkPage($ps,$this->SiteURL);
        $ls = new \fw\LinkSet(\fw\Hostname::ToString($this->Request['Hostname']));
        $msg = new \fw\Messager;
        $vr = new \fw\ValidationReport('error');

        $asmps = new PageSet(fw('asmdb'));
        $asmhtml = new enUSHTMLSet(fw('asmdb'));
        $asmss = new SiteSet(fw('asmthey db'));

        $this->Wire(array('page'=>$page,'ps'=>$ps,'html'=>$html,'lp'=>$lp,'ls'=>$ls,
                          'msg'=>$msg,'vr'=>$vr));

        $html->ConnectWireables($page,$lp,$ls,$msg,$vr);


        $page->Title = URL::Hostname($this->ConsoleURL).' Console';
        $page->Description = 'asmblr Console';
        \fw\Inc::Dir('Routines');
        $html->LoadDir('HTML');


        $ps->Create('RESTv1','/restv1/','\asm\REST::v1');

        $ps->Create('Account','/account/','\asm\AAPI::Account');
        $ps->Create('Site','/site/','\asm\AAPI::Site');

        $ps->Create('AccountCreate','/account/create','\asm\AccountAPI::Create');
        $ps->Create('AccountAuthenticate','/account/authenticate','\asm\AccountAPI::Authenticate');
        $ps->Create('AccountRead','/account/read','\asm\AccountAPI::Read');

        $ps->Create('SiteCreate','/site/create','\asm\SiteAPI::Create');
        $ps->Create('SiteRead','/site/read','\asm\SiteAPI::Read');


        $ps->Create('Home','/','Request::Home');

        $ps->Create('Examples','/examples','Request::Examples',
                    array('html,Article,examples_Home',
                          'html,RightAside,ajf_examples-aside',
                    array('page','Title',"Examples Forms - {$page->Title}")));

        $ps->Create('CSS','/css/','Request::CSSHandler');
        $ps->Create('JS','/js/','Request::JSHandler');

        $ps->Create('JSON','/json/','JSON');
        $ps->Create('AjaxFrags','/ajf/','AjaxFrags');

//        fw('html')->SetRoutine('ajf_examples-aside','Internal::ExamplesAside');


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

    public function GoREST()
    {
        echo 'REST';
    }

    public function Go()
    {
        echo 'These aren\'t the droids you\'re looking for.';
    }


    public function NoName()
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
//         if( isset($_SERVER[$this->DebugToken]) === TRUE )
//             $this->DebugDump('MATCH',\fw\URL::ToString($T),empty($S)?NULL:$S['Name']);

        return $this->asmdb->findOne(array('Domain'=>$Domain));
    }

    // Executing a site involves:
    //  - adding config parameters to this asmSite object
    //  - applying directives (same as PageSet)
    //  - executing the routine (same as PageSet)
    //  - add site templates
    //  - match and execute a page
    //  - render
    // TODO: handle lib code
    public function Execute( $Site )
    {
//        if( isset($_SERVER[$this->DebugToken]) === TRUE )
//            $this->DebugDump('EXECUTE',$Site['URL'],$Domain);

        foreach( $Site['Config'] as $K => $V )
            $this->{$K} = $V;

        $this->CalcURLs();

//            else if( $V['Token'] === 'self' )
//                $this->ApplyDirective($V['Key'],$V['Value']);

        // this is hardwired though could be done via config/directives
        $page = new \fw\KeyValueSet;

        $ps = new \asm\PageSet($this->asmdb);
        $html = new \asm\TemplateSet($this->asmdb,$Site['Domain']);

        $lp = new \fw\LinkPage($ps,$this->SiteURL);
        $ls = new \fw\LinkSet($Site['Domain']);
        $msg = new \fw\Messager;
//        $vr = new \fw\ValidationReport('error');

        $this->Wire(array('page'=>$page,'ps'=>$ps,'html'=>$html,'lp'=>$lp,'ls'=>$ls,'msg'=>$msg));

        $html->ConnectWireables($page,$lp,$ls,$msg);

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
                return $Site['Routine'][0]::$Site['Routine'][1]();
            else
                return eval($Site['Routine'][0]);
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

//        else if( isset($_SERVER[$this->DebugToken]) )
//            throw new \fw\Exception("Page '$Domain' or last match '{$this->Matched['Domain']}' doesn't exist for execution in SiteSet '{$this->WiredAs}'.");
    }

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
}


define('DOC_ROOT',getcwd().DIRECTORY_SEPARATOR);
define('APP_ROOT',str_replace('DOC_ROOT','APP_ROOT',DOC_ROOT));

\fw\Inc::Ext('Mongo.inc');
\fw\Inc::Dir('asmblr');

$asm = new asmblr;

$mongo = new \fw\Mongo;
$asmdb = $mongo->Alias('asmblr','asmdb');
$asm->Wire($asmdb,'asmdb');

$SH = new SessionStoreMongoDB($asmdb);
session_set_save_handler($SH);
session_start();

$Domain = Request::Hostname();

if( $Domain === URL::Hostname($asm->ConsoleURL) )
{
    if( Request::Top() === URL::Top($asm->RESTURL) )
    {
        $asm->GoREST();
    }
    else
    {
        $asm->GoConsole();
    }
}
else
{
    if( ($Site = $ASM->Match($Domain)) !== NULL )
    {
        $ASM->Execute($Site);
    }
    else
    {
        echo 'Invalid request';
        \fw\HTTP::_404();
    }
}


define('TIME_TO_BUILD',round((microtime(TRUE)-START_TIME),4)*1000.000);

