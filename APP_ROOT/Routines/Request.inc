<?php

// don't we need auth checks for things like Site, Page, Template, ?
abstract class Request extends \fw\RoutineSet
{
    public static function Logout()
    {
        $_SESSION['Account'] = array();
        $_SESSION['LoggedIn'] = FALSE;

        fw('lp')->Go('Home');
    }

    public static function Home()
    {
        if( !fw('page')->LoggedIn )
        {
            fw('html')->ReMap('Article','Login');
            return;
        }

        $SS = asm('ss')->Listing(new \MongoId($_SESSION['Account']['_id']));

        fw('html')->Connect($SS,'SS');
        fw('html')->ReMap('Article','Home');
    }

    public static function Site()
    {
        if( !fw('page')->LoggedIn )
            fw('lp')->Go('Home');

        $_id = \fw\Request::Path(-1);
        $S = asm('ss')->Read(new \MongoId($_id));
        if( empty($S) )
            fw()->NoPageHandler();

        $PS = asm('ps')->Listing($S['_id']);
        $TS = asm('ts')->Listing($S['_id']);

        fw('page')->Domain = $S['Domain'];
        fw('html')->Connect($PS,'PS');
        fw('html')->Connect($TS,'TS');
        fw('html')->Connect($S,'S');
        fw('html')->Connect(array_keys(asm()->GetWired()),'DirectiveNames');
        fw('html')->ReMap('Article','Site');

        $DS = new \asm\DataSet(asm('asmdb'),$S['_id'],'Directive');
        fw('html')->Connect($DS,'DS');
    }

    public static function Page()
    {
        if( !fw('page')->LoggedIn )
            fw('lp')->Go('Home');

        $_id = \fw\Request::Path(-1);
        $P = asm('ps')->Read(new \MongoId($_id));
        if( empty($P) )
            fw()->NoPageHandler();

        $S = asm('ss')->Read(new \MongoId($P['Site_id']));

        fw('page')->Domain = $S['Domain'];
        fw('page')->Path = $P['Path'];

        fw('html')->Connect($S,'S');
        fw('html')->Connect($P,'P');

        fw('html')->Connect(array_keys(asm()->GetWired()),'DirectiveNames');
        fw('html')->ReMap('Article','Page');

        $DS = new \asm\DataSet(asm('asmdb'),$S['_id'],'Directive'.$_id);
        fw('html')->Connect($DS,'DS');
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
        $F = 'js_'.str_replace('.js','',\fw\Request::Path(-1));

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
}


