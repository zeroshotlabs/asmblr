<?php
namespace asm;


abstract class REST extends \fw\RoutineSet
{
    protected static $as;
    protected static $ss;
    protected static $ps;
    protected static $html;

    protected static $MatchPath;

    protected static $IsSU = FALSE;
    protected static $Account;
    protected static $Site;
    protected static $Template;
    protected static $Page;
    protected static $Content;

    public static $Response;


    public static function v1( $MatchPath )
    {
        $asmdb = fw('asmdb');
        $asmdb->AccountSet->ensureIndex(array('Email'=>TRUE),array('unique'=>TRUE));
        $asmdb->SiteSet->ensureIndex(array('Domain'=>TRUE),array('unique'=>TRUE));
        $asmdb->SiteSet->ensureIndex(array('AAID'=>TRUE));
        $asmdb->PageSet->ensureIndex(array('Site_id'=>TRUE,'Path'=>TRUE),array('unique'=>TRUE));
        $asmdb->TemplateSet->ensureIndex(array('Site_id'=>TRUE,'Name'=>TRUE),array('unique'=>TRUE));
        $asmdb->ContentSet->ensureIndex(array('Site_id'=>TRUE,'Path'=>TRUE),array('unique'=>TRUE));

        static::$as = $as = fw('as');
        static::$ss = fw('ss');
        static::$ps = fw('ps');
        static::$ts = fw('ts');

        \fw\Path::Del(0,$MatchPath);
        static::$MatchPath = $MatchPath;

        $Op = str_replace('-','_',implode('_',\fw\Path::Top($MatchPath,2)));

        // if the SU header includes our secure token, we're a SU
        if( !empty($_SERVER['HTTP_X_ASMBLR_SU']) && $_SERVER['HTTP_X_ASMBLR_SU'] === 's3cr3tbd' )
            static::$IsSU = TRUE;
        else
            static::$IsSU = FALSE;

        // certain methods can only be done by a SU and no AAID is required
        if( in_array($Op,array('account_read','account_create','account_set_status','account_auth'),TRUE) === TRUE )
        {
            if( static::$IsSU !== TRUE )
                \fw\HTTP::_401();
        }
        // otherwise we must have a valid AAID and matching password (md5()) if not a SU
        else
        {
            if( empty($_SERVER['PHP_AUTH_USER']) )
                \fw\HTTP::_401();
            else if( ($A = $as->Read(new \MongoId($_SERVER['PHP_AUTH_USER']))) === NULL )
                \fw\HTTP::_401();
            else
                static::$Account = $A;

            if( static::$IsSU !== TRUE )
            {
                if( empty($_SERVER['PHP_AUTH_PW']) )
                    \fw\HTTP::_401();

                if( (static::$Account['Password'] !== $_SERVER['PHP_AUTH_PW']) )
                    \fw\HTTP::_401();
            }
        }

        // at this point we have an authenticated account or SU
        // remaining site ownership auth is done at the method
        static::{$Op}();

        if( !empty(static::$Response) )
        {
            // this is sort of a pain in the ass
            foreach( static::$Response['Data'] as $K => &$V )
            {
                if( is_object($V) && $V instanceof \MongoId )
                    $V = (string) $V;
            }

            fw('html')->Connect(static::$Response,'Response');
        }
    }


    public static function __callStatic( $Name,$Args )
    {
        \fw\HTTP::_404();
        exit;
    }

    protected static function AuthSite( \MongoId $S_id )
    {
        if( empty(static::$Account['_id']) )
            \fw\HTTP::_401();

        $S = static::$ss->Read($S_id);
        if( empty($S) )
            \fw\HTTP::_400();

        if( (((string) $S['AAID']) === ((string) static::$Account['_id'])) || (static::$IsSU === TRUE) )
        {
            static::$Site = $S;
        }
        else
            \fw\HTTP::_403();
    }

    protected static function AuthTemplate( \MongoId $T_id )
    {
        $T = static::$ts->Read($T_id);
        if( empty($T) )
            \fw\HTTP::_400();

        static::AuthSite($P['Site_id']);
        static::$Template = $T;
    }

    protected static function AuthPage( \MongoId $P_id )
    {
        $P = static::$ps->Read($P_id);
        if( empty($P) )
            \fw\HTTP::_400();

        static::AuthSite($P['Site_id']);
        static::$Page = $P;
    }

    // SU
    protected static function account_create()
    {
        if( static::$IsSU !== TRUE )
            \fw\HTTP::_403();

        if( !rv::Email() || !rv::Password() || !rv::Name() || !v::Text('Company',$_POST,FALSE) )
            \fw\HTTP::_400();

        $A = Account::Init($_POST['Email'],$_POST['Password'],$_POST['Name'],!empty($_POST['Company'])?$_POST['Company']:'');

        try
        {
            static::$as->Create($A);
            static::$Response = JSONResponse::Ok($A);
        }
        catch( \Exception $E )
        {
            if( static::$as->IsDupeE($E) )
                static::$Response = JSONResponse::Fail('Duplicate Email');
            else
                static::$Response = JSONResponse::Fail('Database error');
        }
    }

    // SU
    protected static function account_read()
    {
        if( static::$IsSU !== TRUE )
            \fw\HTTP::_403();

        $R = array('AAID'=>\fw\Path::Bottom(static::$MatchPath));

        if( v::_id('AAID',$R) )
        {
            $A = static::$as->Read(new \MongoId($R['AAID']));

            if( empty($A) )
                static::$Response = JSONResponse::Fail('Unknown account');
            else
                static::$Response = JSONResponse::Ok($A);
        }
        else
            \fw\HTTP::_400();
    }

    // SU
    protected static function account_set_status()
    {
        if( static::$IsSU !== TRUE )
            \fw\HTTP::_403();

        if( !rv::Status() )
            \fw\HTTP::_400();

        $R = static::$as->SetStatus($_POST['Status'],static::$Account);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Account);
        else
            static::$Reponse = JSONResponse::Fail('Database error');
    }

    // SU
    protected static function account_auth()
    {
        if( static::$IsSU !== TRUE )
            \fw\HTTP::_403();

        if( !rv::Email() || !rv::Password() )
            \fw\HTTP::_400();

        $A = static::$as->Auth($_POST['Email'],$_POST['Password']);

        if( empty($A) )
            static::$Response = JSONResponse::Fail('Invalid credentials.');
        else
            static::$Response = JSONResponse::Ok($A);
    }

    protected static function account_set_password()
    {
        if( !rv::Password() )
            \fw\HTTP::_400();

        $R = static::$as->SetPassword($_POST['Password'],static::$Account);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Account);
        else
            static::$Reponse = JSONResponse::Fail('Database error');
    }

    protected static function account_set_name()
    {
        if( !rv::Name() )
            \fw\HTTP::_400();

        $R = static::$as->SetName($_POST['Name'],static::$Account);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Account);
        else
            static::$Reponse = JSONResponse::Fail('Database error');
    }

    protected static function account_set_company()
    {
        if( !rv::Name('Company') )
            \fw\HTTP::_400();

        $R = static::$as->SetCompany($_POST['Company'],static::$Account);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Account);
        else
            static::$Reponse = JSONResponse::Fail('Database error');
    }


    protected static function site_read()
    {
        static::AuthSite(\fw\Path::Bottom(static::$MatchPath));

        static::$Response = JSONResponse::Ok(static::$Site);
    }

    protected static function site_create()
    {
        if( !rv::Domain() )
            \fw\HTTP::_400();

        $S = Site::Init($_POST['Domain'],static::$Account['_id']);

        try
        {
            static::$ss->Create($S);
            static::$Response = JSONResponse::Ok($S);
        }
        catch( \Exception $E )
        {
            if( static::$ss->IsDupeE($E) )
                static::$Response = JSONResponse::Fail('Duplicate Domain');
            else
                static::$Response = JSONResponse::Fail('Database error');
        }
    }

    protected static function site_set_domain()
    {
        if( !rv::Site_id() || !rv::Domain() )
            \fw\HTTP::_400();

        static::AuthSite($_POST['Site_id']);

        try
        {
            $R = static::$ss->SetDomain($_POST['Domain'],static::$Site);

            if( $R === TRUE )
                static::$Response = JSONResponse::Ok(static::$Site);
            else
                static::$Response = JSONResponse::Fail('Database error');
        }
        catch( \Exception $E )
        {
            if( static::$ss->IsDupeE($E) )
                static::$Response = JSONResponse::Fail('Duplicate Domain');
            else
                static::$Response = JSONResponse::Fail('Database error');
        }
    }

    protected static function site_set_status()
    {
        if( !rv::Site_id() || !rv::Status() )
            \fw\HTTP::_400();

        static::AuthSite($_POST['Site_id']);

        $R = static::$ss->SetStatus($_POST['Status'],static::$Site);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Site);
        else
            static::$Reponse = JSONResponse::Fail('Database error');
    }

    protected static function site_set_baseurl()
    {
        if( !rv::Site_id() || !rv::BaseURL())
            \fw\HTTP::_400();

        static::AuthSite($_POST['Site_id']);

        $R = static::$ss->SetBaseURL($_POST['BaseURL'],static::$Site);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Site);
        else
            static::$Reponse = JSONResponse::Fail('Database error');
    }

    protected static function site_set_routine()
    {
        if( !rv::Site_id() || !rv::Routine() )
            \fw\HTTP::_400();

        static::AuthSite($_POST['Site_id']);

        $R = static::$ss->SetRoutine($_POST['Routine'],static::$Site);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Site);
        else
            static::$Reponse = JSONResponse::Fail('Database error');
    }

    protected static function site_set_directives()
    {
        if( !rv::Site_id() || !rv::Directives() )
            \fw\HTTP::_400();

        static::AuthSite($_POST['Site_id']);

        $R = static::$ss->SetDirectives($_POST['Directives'],static::$Site);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Site);
        else
            static::$Reponse = JSONResponse::Fail('Database error');
    }

    protected static function site_set_config()
    {
        if(!rv::Site_id() || !rv::Config() )
            \fw\HTTP::_400();

        static::AuthSite($_POST['Site_id']);

        $R = static::$ss->SetConfig($_POST['Config'],static::$Site);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Site);
        else
            static::$Reponse = JSONResponse::Fail('Database error');
    }

    protected static function page_read()
    {
        static::AuthPage(\fw\Path::Bottom(static::$MatchPath));

        static::$Response = JSONResponse::Ok(static::$Page);
    }

    protected static function page_delete()
    {
        static::AuthPage(\fw\Path::Bottom(static::$MatchPath));

        if( static::$ps->Delete(static::$Page) )
            static::$Response = JSONResponse::Ok();
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

    protected static function page_create()
    {
        if( !rv::Site_id() || !rv::Path() )
            \fw\HTTP::_400();

        static::AuthSite($_POST['Site_id']);

        $P = Page::Init(static::$Site['_id'],$_POST['Path']);

        try
        {
            static::$ps->Create($P);
            static::$Response = JSONResponse::Ok($P);
        }
        catch( \Exception $E )
        {
            if( static::$ps->IsDupeE($E) )
                static::$Response = JSONResponse::Fail('Duplicate Path');
            else
                static::$Response = JSONResponse::Fail('Database error');
        }
    }

    protected static function page_set_path()
    {
        if( !rv::Page_id() || !rv::Path() )
            \fw\HTTP::_400();

        static::AuthPage($_POST['Page_id']);

        try
        {
            static::$ps->SetPath($_POST['Path'],static::$Page);
            static::$Response = JSONResponse::Ok(static::$Page);
        }
        catch( \Exception $E )
        {
            if( static::$ps->IsDupeE($E) )
                static::$Response = JSONResponse::Fail('Duplicate Path');
            else
                static::$Response = JSONResponse::Fail('Database error');
        }
    }

    protected static function page_set_status()
    {
        if( !rv::Page_id() || !rv::Status() )
            \fw\HTTP::_400();

        static::AuthPage($_POST['Page_id']);

        $R = static::$ps->SetStatus($_POST['Status'],static::$Page);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Page);
        else
            static::$Reponse = JSONResponse::Fail('Database error');
    }

    protected static function page_set_name()
    {
        if( !rv::Page_id() || !rv::Name() )
            \fw\HTTP::_400();

        static::AuthPage($_POST['Page_id']);

        $R = static::$ps->SetName($_POST['Status'],static::$Page);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Page);
        else
            static::$Reponse = JSONResponse::Fail('Database error');
    }

    protected static function page_set_routine()
    {
        if( !rv::Page_id() || !rv::Routine() )
            \fw\HTTP::_400();

        static::AuthPage($_POST['Page_id']);

        $R = static::$ps->SetRoutine($_POST['Routine'],static::$Page);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Page);
        else
            static::$Reponse = JSONResponse::Fail('Database error');
    }

    protected static function page_set_directives()
    {
        if( !rv::Page_id() || !rv::Directives() )
            \fw\HTTP::_400();

        static::AuthPage($_POST['Page_id']);

        $R = static::$ps->SetDirectives($_POST['Directives'],static::$Page);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Page);
        else
            static::$Reponse = JSONResponse::Fail('Database error');
    }

    protected static function template_read()
    {
        static::AuthTemplate(\fw\Path::Bottom(static::$MatchPath));

        static::$Response = JSONResponse::Ok(static::$Template);
    }

    protected static function template_delete()
    {
        static::AuthTemplate(\fw\Path::Bottom(static::$MatchPath));

        if( static::$ts->Delete(static::$Template) )
            static::$Response = JSONResponse::Ok();
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

    protected static function template_create()
    {
        if( !rv::Site_id() || !rv::Name() || !rv::Body() )
            \fw\HTTP::_400();

        static::AuthSite($_POST['Site_id']);

        $T = Template::Init(static::$Site['_id'],$_POST['Name'],$_POST['Body']);

        try
        {
            static::$ts->Create($T);
            static::$Response = JSONResponse::Ok($T);
        }
        catch( \Exception $E )
        {
            if( static::$ts->IsDupeE($E) )
                static::$Response = JSONResponse::Fail('Duplicate Name');
            else
                static::$Response = JSONResponse::Fail('Database error');
        }
    }

    protected static function template_set_name()
    {
        if( !rv::Template_id() || !rv::Name() )
            \fw\HTTP::_400();

        static::AuthTemplate($_POST['Template_id']);

        try
        {
            static::$ts->SetName($_POST['Name'],static::$Template);
            static::$Response = JSONResponse::Ok(static::$Template);
        }
        catch( \Exception $E )
        {
            if( static::$ps->IsDupeE($E) )
                static::$Response = JSONResponse::Fail('Duplicate Name');
            else
                static::$Response = JSONResponse::Fail('Database error');
        }
    }

    protected static function template_set_body()
    {
        if( !rv::Template_id() || !rv::Body() )
            \fw\HTTP::_400();

        static::AuthTemplate($_POST['Template_id']);

        $R = static::$ps->SetRoutine($_POST['Routine'],static::$Template);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Template);
        else
            static::$Reponse = JSONResponse::Fail('Database error');
    }

    protected static function template_set_routine()
    {
        if( !rv::Template_id() || !rv::Routine() )
            \fw\HTTP::_400();

        static::AuthTemplate($_POST['Template_id']);

        $R = static::$ps->SetRoutine($_POST['Routine'],static::$Template);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Template);
        else
            static::$Reponse = JSONResponse::Fail('Database error');
    }

}


