<?php

// don't we need auth checks for things like Site, Page, Template, ?
// we're back and forth between asm() and fw()
abstract class Console extends \fw\RoutineSet
{
    public static function Logout()
    {
        $_SESSION['Account'] = array();
        $_SESSION['LoggedIn'] = FALSE;

        fw('lp')->Go('Home');
    }

    public static function Home()
    {
        fw('html')->ReMap('LeftNav',NULL);

        if( !fw('page')->LoggedIn )
        {
            fw('html')->ReMap('Article','Login');
            fw('html')->Stack('jss_login','jsready');
            return;
        }

        $SS = asm('ss')->Listing(new \MongoId($_SESSION['Account']['_id']));

        fw('html')->Connect($SS,'SS');
        fw('html')->ReMap('Article','Home');
        fw('html')->Stack('jss_createsite','jsready');
    }

    public static function Site()
    {
        if( !fw('page')->LoggedIn )
            fw('lp')->Go('Home');

        static::InitSite(new \MongoId(\fw\Request::Path(-1)));

        fw('page')->ActiveNav = 'Site';
        fw('html')->Stack('jss_site','jsready');
    }

    public static function Page()
    {
        if( !fw('page')->LoggedIn )
            fw('lp')->Go('Home');

        static::InitPage(new \MongoId(\fw\Request::Path(-1)));

        fw('page')->ActiveNav = 'Page';
        fw('html')->Stack('jss_page','jsready');
    }

    public static function Template()
    {
        if( !fw('page')->LoggedIn )
            fw('lp')->Go('Home');

        static::InitTemplate(new \MongoId(\fw\Request::Path(-1)));

        fw('page')->ActiveNav = 'Template';
        fw('html')->Stack('jss_template','jsready');
    }

    // serve out HTML fragments for ajax requests
    // we don't use HTML/Javascript to generate itself because they're not designed for it
    // PHP is in case people haven't noticed; however, this structure may not be ideal since it
    // sort of recreates our top-level page/routine/template behavior - perhaps a specialized
    // pageset/template is the answer
    // for the below we may want someway to allow the user to
    // URL: /ajf/some-html-fragment-name
    public static function ajfHandler()
    {
        fw('html')->SetRoutine('ajf_directive_grid','Console::directive_grid');

        $F = 'ajf_'.\fw\Request::Path(-1);

        if( isset(fw('html')->{$F}) )
        {
            fw('html')->ReMap('Base',$F);
        }
        else
        {
            // kill the render
            fw('html')->ReMap('Base',NULL);
            \fw\HTTP::_404();
        }
    }

    // for the AJFs - auth?
    // since this is a Template Routine we can only return FALSE, not remap to NULL
    public static function directive_grid()
    {
        fw('html')->Connect(asm()->DirWireables,'DirectiveNames');

        if( \asm\rv::Page_id() )
        {
            $P = fw('asmps')->Read_id(new \MongoId($_POST['Page_id']));
            if( empty($P) )
                \fw\HTTP::_400();

            $ds = new \asm\DirectiveSet(asm('asmdb'),$P['Site_id']);
            fw('html')->Connect($ds->PageList($P),'DS');
        }
        else if( \asm\rv::Site_id() )
        {
            $S = asm('ss')->Read(new \MongoId($_POST['Site_id']));
            if( empty($S) )
                \fw\HTTP::_400();

            $ds = new \asm\DirectiveSet(asm('asmdb'),$S['_id']);
            fw('html')->Connect($ds->SiteList($S),'DS');
        }
        else
            \fw\HTTP::_400();


        return;


        // order is important
        if( \asm\rv::Page_id() )
        {
            $S = fw('asmps')->Read_id(new \MongoId($_POST['Page_id']));
            $Tag = 'DirectiveP_';
        }
        else if( \asm\rv::Site_id() )
        {
            $S = asm('ss')->Read(new \MongoId($_POST['Site_id']));
            $Tag = 'Directive';
        }
        else
            \fw\HTTP::_400();

        if( empty($S) )
        {
            \fw\HTTP::_404();
            return FALSE;
        }

        if( isset($S['Site_id']) )
        {
            $Site_id = $S['Site_id'];
            $Tag = $Tag.$S['_id'];
        }
        else
            $Site_id = $S['_id'];

        $DS = new \asm\DataSet(asm('asmdb'),$Site_id,$Tag);
        fw('html')->Connect($DS,'DS');
    }

    public static function CSSHandler()
    {
        $F = 'css_'.str_replace('.css','',\fw\Request::Path(-1));

        if( isset(fw('html')->{$F}) )
        {
            \fw\HTTP::ContentType('css','utf-8');
            fw('html')->ReMap('Base',$F);
        }
        else
        {
            fw()->NoPageHandler();
        }
    }

    public static function JSHandler()
    {
        $F = 'jsext_'.str_replace('.js','',\fw\Request::Path(-1));

        if( isset(fw('html')->{$F}) )
        {
            \fw\HTTP::ContentType('js','utf-8');
            fw('html')->ReMap('Base',$F);
        }
        else
        {
            fw()->NoPageHandler();
        }
    }

    protected static function InitSite( \MongoId $S )
    {
        $S = asm('ss')->Read($S);
        if( empty($S) )
            fw()->NoPageHandler();

        fw('page')->Domain = $S['Domain'];

        // the beginnings of live edit?  may better be aggregated up somewhere else
        // bring our asmblr stuff online - no site execution happens though (Go())
        // we forge the request data to appear to hit the site; though we need a way
        // honor the site's BaseURL (like https and path), though we run into a chicken-egg problem
        // so it's hard wired for now
        $R = \asm\Request::Init('http://'.$S['Domain']);
        asm()->GetSet($S['Domain'],$R);

        $lp = new \asm\LinkPage(asm('ps'),asm()->SiteURL);
        $ls = new \fw\LinkSet($S['Domain']);

        asm()->Wire(array('lp'=>$lp,'ls'=>$ls));

        // should these be API calls?
        $PL = fw('asmps')->Listing($S['_id']);
        $TL = fw('asmts')->Listing($S['_id']);

        fw('html')->Connect($PL,'PL');
        fw('html')->Connect($TL,'TL');
        fw('html')->Connect($S,'S');

        fw('html')->Stack('jss_leftnav','jsready');
    }

    protected static function InitPage( \MongoId $P )
    {
        $P = fw('asmps')->Read_id($P);
        if( empty($P) )
            fw()->NoPageHandler();

        static::InitSite($P['Site_id']);

        fw('html')->Connect($P,'P');
    }

    protected static function InitTemplate( \MongoId $T )
    {
        $T = fw('asmts')->Read_id($T);
        if( empty($T) )
            fw()->NoPageHandler();

        static::InitSite($T['Site_id']);

        fw('html')->Connect($T,'T');
    }

}

