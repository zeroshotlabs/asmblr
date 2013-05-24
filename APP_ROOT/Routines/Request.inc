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
    }

    public static function Template()
    {
        if( !fw('page')->LoggedIn )
            fw('lp')->Go('Home');

        $_id = \fw\Request::Path(-1);
        $T = asm('ts')->Read(new \MongoId($_id));
        if( empty($T) )
            fw()->NoPageHandler();

        $S = asm('ss')->Read(new \MongoId($T['Site_id']));

        fw('page')->Domain = $S['Domain'];
        fw('page')->Name = $T['Name'];

        fw('html')->Connect($S,'S');
        fw('html')->Connect($T,'T');

        fw('html')->ReMap('Article','Template');
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
        fw('html')->SetRoutine('ajf_directive_table','Request::directive_table');

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
    public static function directive_table()
    {
        if( \asm\rv::Site_id() )
        {
            $S = asm('ss')->Read(new \MongoId($_POST['Site_id']));
            $Tag = 'Directive';
        }
        else if( \asm\rv::Page_id() )
        {
            $S = asm('ps')->Read(new \MongoId($_POST['Page_id']));
            $Tag = 'DirectiveP_';
        }
/*
        // wait do templates have directives?  not yet anyway...
        else if( \asm\rv::Template_id() )
        {
            $S = asm('ts')->Read(new \MongoId($_POST['Template_id']));
            $Tag = 'DirectiveT_';
        }
*/
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


