<?php
namespace asm;


// For other cases, such as RESTful APIs, it may be useful to
// read the raw request directly from stdin.
// static::$JSON = json_decode(file_get_contents('php://input'),TRUE);

class AAPI extends \fw\RoutineSet
{
    protected static $as;
    protected static $ss;
    protected static $ps;
    protected static $ts;
    protected static $cs;

    protected static $MatchPath;

    protected static $IsSU = FALSE;
    protected static $Account;
    protected static $Site;
    protected static $Page;
    protected static $Template;
    protected static $Content;

    public static $Response;


    public static function v1( $MatchPath )
    {
        $asmdb = asm('asmdb');
        $asmdb->AccountSet->ensureIndex(array('Email'=>TRUE),array('unique'=>TRUE));

        $asmdb->SiteSet->ensureIndex(array('Domain'=>TRUE),array('unique'=>TRUE));
        $asmdb->SiteSet->ensureIndex(array('AAID'=>TRUE));

        $asmdb->PageSet->ensureIndex(array('Site_id'=>TRUE,'Path'=>TRUE),array('unique'=>TRUE));
        $asmdb->PageSet->ensureIndex(array('Site_id'=>TRUE,'Name'=>TRUE),array('unique'=>TRUE));

        $asmdb->TemplateSet->ensureIndex(array('Site_id'=>TRUE,'Name'=>TRUE),array('unique'=>TRUE));

        $asmdb->ContentSet->ensureIndex(array('Site_id'=>TRUE,'Path'=>TRUE),array('unique'=>TRUE));
//        $asmdb->ContentSet->ensureIndex(array('Site_id'=>TRUE,'Name'=>TRUE),array('unique'=>TRUE));

        static::$as = $as = asm('as');
        static::$ss = asm('ss');
        // use standalone sets from our console regular framewire app
        static::$ps = fw('asmps');
        static::$ts = fw('asmts');
        static::$cs = fw('asmcs');

        \fw\Path::Del(0,$MatchPath);
        static::$MatchPath = $MatchPath;

        $Op = str_replace('-','_',implode('_',\fw\Path::Top($MatchPath,2)['Segments']));

        // if the SU header includes our secure token, we're a SU
        if( !empty($_SERVER['HTTP_X_ASMBLR_SU']) && $_SERVER['HTTP_X_ASMBLR_SU'] === 's3cr3tbd' )
            static::$IsSU = TRUE;
        else
            static::$IsSU = FALSE;

        // certain methods can only be done by a SU and no AAID is required
        if( in_array($Op,array('account_read','account_create','account_set_status'),TRUE) === TRUE )
        {
            // quicky hack - see account_create
//            if( static::$IsSU !== TRUE )
//                \fw\HTTP::_401();
        }
        // account_auth and util methods are public
        else if( $Op === 'account_auth' || $Op === 'util_site_statuses' || $Op === 'util_dir_names' )
        {
        }
        // otherwise we must have a valid AAID and matching password (md5()) if not a SU
        else
        {
            if( empty($_SERVER['HTTP_X_ASMBLR_USER']) )
                \fw\HTTP::_401();
            else if( ($A = $as->Read(new \MongoId($_SERVER['HTTP_X_ASMBLR_USER']))) === NULL )
                \fw\HTTP::_401();
            else
                static::$Account = $A;

            if( static::$IsSU !== TRUE )
            {
                if( empty($_SERVER['HTTP_X_ASMBLR_PW']) )
                    \fw\HTTP::_401();

                if( (static::$Account['Password'] !== $_SERVER['HTTP_X_ASMBLR_PW']) )
                    \fw\HTTP::_401();
            }
        }

        // at this point we have an authenticated account or SU
        // remaining site ownership auth is done at the method
        static::{$Op}();

        if( !empty(static::$Response['Data']) && is_array(static::$Response['Data']) )
        {
            // this is sort of a pain in the ass
            foreach( static::$Response['Data'] as $K => &$V )
            {
                if( is_object($V) && $V instanceof \MongoId )
                    $V = (string) $V;
            }
        }

        fw('html')->Connect(empty(static::$Response)?array():static::$Response,'Response');
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

    protected static function AuthPage( \MongoId $P_id )
    {
        $P = static::$ps->Read_id($P_id);

        if( empty($P) )
            \fw\HTTP::_400();

        static::AuthSite($P['Site_id']);
        static::$Page = $P;
    }

    protected static function AuthTemplate( \MongoId $T_id )
    {
        $T = static::$ts->Read_id($T_id);
        if( empty($T) )
            \fw\HTTP::_400();

        static::AuthSite($T['Site_id']);
        static::$Template = $T;
    }

    protected static function AuthContent( \MongoId $C_id )
    {
        $C = static::$cs->Read_id($C_id);
        if( empty($C) )
            \fw\HTTP::_400();

        static::AuthSite($C['Site_id']);
        static::$Content = $C;
    }

    // public - might not be needed
    protected static function util_site_statuses()
    {
        $N = array();
        foreach( asm()->SiteStatuses as $V )
            $N[] = array('value'=>$V,'text'=>$V);

        static::$Response = $N;
    }

    // public - hard wired "security" so that we only reveal
    // certain wireables that can be used, but real security should be considered
    protected static function util_dir_names()
    {
        $N = array();
        foreach( asm()->DirWireables as $V )
            $N[] = array('value'=>$V,'text'=>$V);

//         // this will need revisiting/security/etc
//         $N = array();
//         foreach( asm()->GetWired() as $K => $V )
//             $N[] = array('value'=>$K,'text'=>$K);

        static::$Response = $N;
    }


    // PUBLIC - WE ALSO START A SESSION which ties us together with the console webapp
    // among some other things like LinkAAPI - when we break console apart from REST,
    // we'd have a console method Login() which calls this via curl (which would have the SU key) and we can
    // remove the session from here - so this is a temp. convience hack - ditto for register/account_create
    protected static function account_auth()
    {
        if( !rv::Email() || !rv::Password() )
            \fw\HTTP::_400();

        $A = static::$as->Auth($_POST['Email'],$_POST['Password']);

        if( empty($A) )
            static::$Response = JSONResponse::Fail('Invalid credentials.');
        else
        {
            session_set_save_handler(new SessionStoreMongoDB(asm('asmdb')));
            session_start();
            $_SESSION['LoggedIn'] = TRUE;
            $_SESSION['Account'] = $A;

            static::$Response = JSONResponse::Ok($A);
        }
    }

    // SU
    protected static function account_create()
    {
        // quicky hack - see account_auth() too
//         if( static::$IsSU !== TRUE )
//             \fw\HTTP::_403();

        if( !rv::Email() || !rv::Password() || !rv::Name() || !v::Text('Company',$_POST,FALSE) )
            \fw\HTTP::_400();

        $A = Account::Init($_POST['Email'],$_POST['Password'],$_POST['Name'],!empty($_POST['Company'])?$_POST['Company']:'');

        mail('hz@stackware.com','asmblr registration',\fw\Debug::Dump($A).\fw\Debug::Dump($_SERVER));

        try
        {
            static::$as->Create($A);
            // hack per above
            static::account_auth();
//            static::$Response = JSONResponse::Ok($A);
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
            static::$Response = JSONResponse::Fail('Database error');
    }

    protected static function account_set_password()
    {
        if( !rv::Password() )
            \fw\HTTP::_400();

        $R = static::$as->SetPassword($_POST['Password'],static::$Account);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Account);
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

    protected static function account_set_name()
    {
        if( !rv::Name() )
            \fw\HTTP::_400();

        $R = static::$as->SetName($_POST['Name'],static::$Account);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Account);
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

    protected static function account_set_company()
    {
        if( !rv::Name('Company') )
            \fw\HTTP::_400();

        $R = static::$as->SetCompany($_POST['Company'],static::$Account);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Account);
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

    protected static function site_read()
    {
        static::AuthSite(new \MongoId(\fw\Path::Bottom(static::$MatchPath)));

        static::$Response = JSONResponse::Ok(static::$Site);
    }

    protected static function site_delete()
    {
        static::AuthSite(new \MongoId(\fw\Path::Bottom(static::$MatchPath)));

        if( static::$ss->Delete(static::$Site) )
        {
            foreach( static::$ps->Listing(static::$Site['_id']) as $P )
                static::$ps->Delete($P);

            foreach( static::$ts->Listing(static::$Site['_id']) as $T )
                static::$ts->Delete($T);

            foreach( static::$cs->Listing(static::$Site['_id']) as $C )
                static::$cs->Delete($C);

            static::$Response = JSONResponse::Ok('Deleted');
        }
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

    // export/import requires --enable-zip compiled PHP
    protected static function site_export()
    {
        static::AuthSite(new \MongoId(\fw\Path::Bottom(static::$MatchPath)));

        fw('html')->ReMap('Base',NULL);

        $Site = static::$Site;

        $Filename = $Site['Domain'].'-'.date('Y-m-d-H-i').'.zip';

        $Z = new \ZipArchive;
        if( $Z->open("/tmp/$Filename",\ZIPARCHIVE::CREATE) !== TRUE )
        {
            static::$Response = JSONResponse::Fail("Failed to create tmp zip file at /tmp/{$Filename}.");
            return;
        }

        $Z->addEmptyDir('Pages');
        $Z->addEmptyDir('Templates');
        $Z->addEmptyDir('Content');

        $Z->addFromString('Site.inc',Site::ToPHP($Site));

        foreach( static::$ps->Listing($Site['_id']) as $P )
        {
            static::$ps->Flatten($P);
            $Z->addFromString("Pages/{$P['Name']}.inc",Page::ToPHP($P));
        }

        foreach( static::$ts->Listing($Site['_id']) as $T )
            $Z->addFromString("Templates/{$T['Name']}.inc",Template::ToPHP($T));

        // for very large sites, we'd want a separate content export most likely
        // this also doesn't save any meta/etc - only content as files with full path
        if( empty($_GET['nocontent']) )
        {
            foreach( static::$cs->Listing($Site['_id'],FALSE,TRUE) as $C )
            {
                // this, and what we use in ContentSrv, could be refactored into GetBody/SendBody for
                // unified delivery or saving of the content - this is basically duped from ContentSRv
                // it would also need to deal with S3/GAE in addition to Mongo
                if( is_object($C['Body']) )
                    $Z->addFromString("Content/{$C['Path']}",$C['Body']->bin);
                else
                    $Z->addFromString("Content/{$C['Path']}",$C['Body']);
            }
        }

        $Z->close();

        $R = array('Filename'=>$Filename,'Content'=>utf8_encode(file_get_contents("/tmp/$Filename")));
        unlink("/tmp/$Filename");

        static::$Response = JSONResponse::Ok($R);
    }

    // uses regular file upload so must be sent as such file upload POST
    protected static function site_import()
    {
        if( !rv::Domain() )
            \fw\HTTP::_400();

        $F = \fw\FileUpload::Init('import_file');
        foreach( $F as $FU )
        {
            if( !\fw\FileUpload::IsOk($FU) )
                continue;

            $S = SiteSet::SiteZip2Arr($FU['TmpPath'],$_POST['Domain'],static::$Account['_id']);

            if( empty($S['Site']) )
            {
                static::$Response = JSONResponse::Fail('Invalid zip - empty Site.inc');
                return;
            }

            SiteSet::SiteArr2Mongo($S,static::$ss,static::$ps,static::$ts,static::$cs);

            static::$Response = JSONResponse::Ok($S['Site']);
        }
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
        if( !rv::Site_id() )
            \fw\HTTP::_400();

        static::AuthSite(new \MongoId($_POST['Site_id']));

        if( !rv::Domain() )
        {
            static::$Response = JSONResponse::Fail('Domain is required.');
            return;
        }

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
                static::$Response = JSONResponse::Fail('Domain exists.');
            else
                static::$Response = JSONResponse::Fail('Database error');
        }
    }

    protected static function site_set_status()
    {
        if( !rv::Site_id() )
            \fw\HTTP::_400();

        static::AuthSite(new \MongoId($_POST['Site_id']));

        if( !rv::Status() )
        {
            static::$Response = JSONResponse::Fail('Unknown status.');
            return;
        }

        $R = static::$ss->SetStatus($_POST['Status'],static::$Site);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Site);
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

    protected static function site_set_baseurl()
    {
        if( !rv::Site_id() || !rv::BaseURL())
            \fw\HTTP::_400();

        static::AuthSite(new \MongoId($_POST['Site_id']));

        $R = static::$ss->SetBaseURL($_POST['BaseURL'],static::$Site);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Site);
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

    protected static function site_set_routine()
    {
        if( !rv::Site_id() || !rv::Routine() )
            \fw\HTTP::_400();

        static::AuthSite(new \MongoId($_POST['Site_id']));

        $R = static::$ss->SetRoutine($_POST['Routine'],static::$Site);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Site);
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

    // will need directive set specific security
    protected static function site_set_directive()
    {
        if( !rv::Site_id() || !rv::Directive() )
            \fw\HTTP::_400();

        static::AuthSite(new \MongoId($_POST['Site_id']));

        // if a Directive_id is present and valid, this will be an update; insert otherwise
        if( !empty($_POST['D_id']) && v::_id('D_id',$_POST) )
            $D_id = new \MongoId($_POST['D_id']);
        else
            $D_id = NULL;

        $D = array('Name'=>$_POST['Name'],'Key'=>$_POST['Key'],'Value'=>$_POST['Value']);
        $R = static::$ss->SetDirective($D,static::$Site,$D_id);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Site);
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

    protected static function site_del_directive()
    {
        if( !rv::Site_id() || !v::_id('D_id',$_POST) )
            \fw\HTTP::_400();

        static::AuthSite(new \MongoId($_POST['Site_id']));

        $D_id = new \MongoId($_POST['D_id']);

        $R = static::$ss->DeleteDirective($D_id,static::$Site);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Site);
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

    protected static function site_cp_directive()
    {
        if( !rv::Site_id() || !v::_id('D_id',$_POST) )
            \fw\HTTP::_400();

        static::AuthSite(new \MongoId($_POST['Site_id']));

        $D_id = new \MongoId($_POST['D_id']);

        $D = static::$ss->CopyDirective($D_id,static::$Site);

        if( empty($D) )
            static::$Response = JSONResponse::Fail('Database error');
        else
            static::$Response = JSONResponse::Ok($D);
    }

    protected static function site_mv_directive()
    {
        if( !rv::Site_id() || !v::_id('D_id',$_POST) )
            \fw\HTTP::_400();

        static::AuthSite(new \MongoId($_POST['Site_id']));

        $D_id = new \MongoId($_POST['D_id']);
        if( v::_id('NextD_id',$_POST) )
            $NextD_id = new \MongoId($_POST['NextD_id']);
        else
            $NextD_id = NULL;

        $D = static::$ss->MoveDirective($D_id,$NextD_id,static::$Site);

        if( empty($D) )
            static::$Response = JSONResponse::Fail('Database error');
        else
            static::$Response = JSONResponse::Ok($D);
    }
/*
    protected static function site_set_config()
    {
        if(!rv::Site_id() || !rv::Config() )
            \fw\HTTP::_400();

        static::AuthSite(new \MongoId($_POST['Site_id']));

        $R = static::$ss->SetConfig($_POST['Config'],static::$Site);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Site);
        else
            static::$Response = JSONResponse::Fail('Database error');
    }
*/

    protected static function page_read()
    {
        static::AuthPage(new \MongoId(\fw\Path::Bottom(static::$MatchPath)));

        static::$Response = JSONResponse::Ok(static::$Page);
    }

    protected static function page_delete()
    {
        static::AuthPage(new \MongoId(\fw\Path::Bottom(static::$MatchPath)));

        if( static::$ps->Delete(static::$Page) )
            static::$Response = JSONResponse::Ok('Deleted');
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

    // pages must have a unique name for linkpage purposes, and of course path
    protected static function page_create()
    {
        if( !rv::Site_id() || !rv::Path() || !rv::Name() )
            \fw\HTTP::_400();

        static::AuthSite(new \MongoId($_POST['Site_id']));

        $P = Page::Init(static::$Site['_id'],$_POST['Name'],$_POST['Path']);

        try
        {
            static::$ps->Create($P);
            static::$Response = JSONResponse::Ok($P);
        }
        catch( \Exception $E )
        {
            if( static::$ps->IsDupeE($E) )
                static::$Response = JSONResponse::Fail('Duplicate Path or Name');
            else
                static::$Response = JSONResponse::Fail('Database error');
        }
    }

    // overwrites an existing with details from a raw PHP representation of a page
    // no merge is done - probably should require SU or something - Name must be specified
    // explicitly as a parameter
    // will do this later if needed
//     protected static function page_set_php()
//     {

//     }


    protected static function page_set_name()
    {
        if( !rv::Page_id() || !rv::Name() )
            \fw\HTTP::_400();

        static::AuthPage(new \MongoId($_POST['Page_id']));

        try
        {
            static::$ps->SetName($_POST['Name'],static::$Page);
            static::$Response = JSONResponse::Ok(static::$Page);
        }
        catch( \Exception $E )
        {
            if( static::$ps->IsDupeE($E) )
                static::$Response = JSONResponse::Fail('Duplicate Name');
            else
                static::$Response = JSONResponse::Fail('Database error');
        }
    }

    protected static function page_set_path()
    {
        if( !rv::Page_id() || !rv::Path() )
            \fw\HTTP::_400();

        static::AuthPage(new \MongoId($_POST['Page_id']));

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

        static::AuthPage(new \MongoId($_POST['Page_id']));

        $R = static::$ps->SetStatus($_POST['Status'],static::$Page);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Page);
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

    protected static function page_set_routine()
    {
        if( !rv::Page_id() || !rv::Routine() )
            \fw\HTTP::_400();

        static::AuthPage(new \MongoId($_POST['Page_id']));

        $R = static::$ps->SetRoutine($_POST['Routine'],static::$Page);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Page);
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

    // will need directive set specific security
    protected static function page_set_directive()
    {
        if( !rv::Page_id() || !rv::Directive() )
            \fw\HTTP::_400();

        static::AuthPage(new \MongoId($_POST['Page_id']));

        // if a Directive_id is present and valid, this will be an update; insert otherwise
        if( !empty($_POST['D_id']) && v::_id('D_id',$_POST) )
            $D_id = new \MongoId($_POST['D_id']);
        else
            $D_id = NULL;

        $D = array('Name'=>$_POST['Name'],'Key'=>$_POST['Key'],'Value'=>$_POST['Value']);
        $R = static::$ps->SetDirective($D,static::$Page,$D_id);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Page);
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

    protected static function page_del_directive()
    {
        if( !rv::Page_id() || !v::_id('D_id',$_POST) )
            \fw\HTTP::_400();

        static::AuthPage(new \MongoId($_POST['Page_id']));

        $D_id = new \MongoId($_POST['D_id']);

        $R = static::$ps->DeleteDirective($D_id,static::$Page);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Page);
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

    protected static function page_cp_directive()
    {
        if( !rv::Page_id() || !v::_id('D_id',$_POST) )
            \fw\HTTP::_400();

        static::AuthPage(new \MongoId($_POST['Page_id']));

        $D_id = new \MongoId($_POST['D_id']);

        $D = static::$ps->CopyDirective($D_id,static::$Page);

        if( empty($D) )
            static::$Response = JSONResponse::Fail('Database error');
        else
            static::$Response = JSONResponse::Ok($D);
    }

    protected static function page_mv_directive()
    {
        if( !rv::Page_id() || !v::_id('D_id',$_POST) )
            \fw\HTTP::_400();

        static::AuthPage(new \MongoId($_POST['Page_id']));

        $D_id = new \MongoId($_POST['D_id']);
        if( v::_id('NextD_id',$_POST) )
            $NextD_id = new \MongoId($_POST['NextD_id']);
        else
            $NextD_id = NULL;

        $D = static::$ps->MoveDirective($D_id,$NextD_id,static::$Page);

        if( empty($D) )
            static::$Response = JSONResponse::Fail('Database error');
        else
            static::$Response = JSONResponse::Ok($D);
    }

    protected static function template_read()
    {
        static::AuthTemplate(new \MongoId(\fw\Path::Bottom(static::$MatchPath)));

        static::$Response = JSONResponse::Ok(static::$Template);
    }

    protected static function template_delete()
    {
        static::AuthTemplate(new \MongoId(\fw\Path::Bottom(static::$MatchPath)));

        if( static::$ts->Delete(static::$Template) )
            static::$Response = JSONResponse::Ok('Deleted');
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

    protected static function template_create()
    {
        if( !rv::Site_id() || !rv::Name() || !rv::Body() )
            \fw\HTTP::_400();

        static::AuthSite(new \MongoId($_POST['Site_id']));

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

        static::AuthTemplate(new \MongoId($_POST['Template_id']));

        try
        {
            static::$ts->SetName($_POST['Name'],static::$Template);
            static::$Response = JSONResponse::Ok(static::$Template);
        }
        catch( \Exception $E )
        {
            if( static::$ts->IsDupeE($E) )
                static::$Response = JSONResponse::Fail('Duplicate Name');
            else
                static::$Response = JSONResponse::Fail('Database error');
        }
    }

    protected static function template_set_routine()
    {
        if( !rv::Template_id() || !rv::Routine() )
            \fw\HTTP::_400();

        static::AuthTemplate(new \MongoId($_POST['Template_id']));

        $R = static::$ts->SetRoutine($_POST['Routine'],static::$Template);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Template);
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

    protected static function template_set_body()
    {
        if( !rv::Template_id() || !rv::Body() )
            \fw\HTTP::_400();

        static::AuthTemplate(new \MongoId($_POST['Template_id']));

        $R = static::$ts->SetBody($_POST['Body'],static::$Template);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Template);
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

// encoding issues re json_encode warnings will need to be dealt with here, and template,
// and in general
    protected static function content_read()
    {
        static::AuthContent(new \MongoId(\fw\Path::Bottom(static::$MatchPath)));

        static::$Response = JSONResponse::Ok(static::$Content);
    }

    protected static function content_delete()
    {
        static::AuthContent(new \MongoId(\fw\Path::Bottom(static::$MatchPath)));

        if( static::$cs->Delete(static::$Content) )
            static::$Response = JSONResponse::Ok('Deleted');
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

    // this will detect and handle a direct POST or a file upload (multipart/form-data via $_FILES)
    // perhaps we should switch this to PUT?  also look at content_set_body
    // this and set_body will also need a large upload/post size limit in php
    protected static function content_create()
    {
        if( !rv::Site_id() || !rv::Path() || (!rv::Body() && !empty($_FILES['Body'])) )
            \fw\HTTP::_400();

        static::AuthSite(new \MongoId($_POST['Site_id']));

        if( !empty($_FILES['Body']) )
            $C = Content::Init(static::$Site['_id'],$_POST['Path'],file_get_contents($_FILES['Body']['tmp_name']));
        else
            $C = Content::Init(static::$Site['_id'],$_POST['Path'],$_POST['Body']);

        try
        {
            static::$cs->Create($C);
            // recurring issue - json_encode() will die with binary body so we kill it for now
            // needs attention for content_ready, set body, etc
            $C['Body'] = 'NOT INCLUDED';
            static::$Response = JSONResponse::Ok($C);
        }
        catch( \Exception $E )
        {
            if( static::$cs->IsDupeE($E) )
                static::$Response = JSONResponse::Fail('Duplicate Path');
            else
                static::$Response = JSONResponse::Fail('Database error');
        }
    }

    protected static function content_set_path()
    {
        if( !rv::Content_id() || !rv::Path() )
            \fw\HTTP::_400();

        static::AuthContent(new \MongoId($_POST['Content_id']));

        try
        {
            static::$cs->SetPath($_POST['Path'],static::$Content);
            static::$Response = JSONResponse::Ok(static::$Content);
        }
        catch( \Exception $E )
        {
            if( static::$cs->IsDupeE($E) )
                static::$Response = JSONResponse::Fail('Duplicate Path');
            else
                static::$Response = JSONResponse::Fail('Database error');
        }
    }

    protected static function content_set_status()
    {
        if( !rv::Content_id() || !rv::Status() )
            \fw\HTTP::_400();

        static::AuthContent(new \MongoId($_POST['Content_id']));

        $R = static::$cs->SetStatus($_POST['Status'],static::$Content);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Content);
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

    // this will replace a body with empty if Body doesn't exist
    // this will not work for browser file uploads
    // for binary files especially, we probably don't want to send back the
    // whole body - here and elsewhere
    protected static function content_set_body()
    {
        if( !rv::Content_id() || !rv::Body() )
            \fw\HTTP::_400();

        static::AuthContent(new \MongoId($_POST['Content_id']));

        $R = static::$cs->SetBody($_POST['Body'],static::$Content);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Content);
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

    protected static function content_set_type()
    {
        // the content type is required - this type of thing probably needs to be fixed elsewhere
        if( !rv::Content_id() || !rv::ContentType() || empty($_POST['Type']) )
            \fw\HTTP::_400();

        static::AuthContent(new \MongoId($_POST['Content_id']));

        $R = static::$cs->SetType($_POST['Type'],static::$Content);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Content);
        else
            static::$Response = JSONResponse::Fail('Database error');
    }

    protected static function content_set_meta()
    {
        // we don't actually care what Meta is but is should be a set of key/value pairs
        if( empty($_POST['Meta']) )
            $_POST['Meta'] = array();

        if( !rv::Content_id() )
            \fw\HTTP::_400();

        static::AuthContent(new \MongoId($_POST['Content_id']));

        $R = static::$cs->SetMeta($_POST['Meta'],static::$Content);

        if( $R === TRUE )
            static::$Response = JSONResponse::Ok(static::$Content);
        else
            static::$Response = JSONResponse::Fail('Database error');
    }
}


// as we might split things up more, this probably belongs under a the console code base strictly speaking
class LinkAAPI extends \fw\LinkSet
{
    protected $Methods = array();


    public function __construct( $BaseURL )
    {
        $this->SetBaseURL($BaseURL);

        $C = new \ReflectionClass('\\asm\AAPI');
        foreach( $C->getMethods() as $M )
        {
            $P = strpos($M->name,'_');
            if( empty($P) )
                continue;
            $this->Methods[$M->name] = \fw\Path::Init('/'.substr($M->name,0,$P).'/'.substr($M->name,$P+1));
        }
    }

    public function __invoke( $Name = NULL,$Set = array() )
    {
        if( isset($this->Methods[$Name]) )
        {
            $Base = $this->BaseURL;
            \fw\Path::Merge($this->Methods[$Name],$Base['Path']);

            if( !empty($Set) )
                \fw\URL::Set($Set,$Base);

            return \fw\URL::ToString($Base);
        }
        else
            return 'not-found';
    }
}


// and this too - also interchangeably use the terms AAPI and REST
// we should probably also have a nice tidy class like this in Javascript
// we'll also need to add SU support here
// curl resource is reused
// DONT FORGET TO HAVE DNS FOR THE ENDPOINT OTHERWISE CURL JUST FAILS
class AAPIClient
{
    protected $la;
    protected $CH;

//    public $Log = '';
    public $ConnectionError = '';
    public $Response = '';


    public function __construct( \asm\LinkAAPI $la,$User = NULL,$PW = NULL )
    {
        if( empty($User) )
        {
            if( empty($_SESSION['Account']['_id']) )
                throw new Exception('AAPI User not present in session - must specify.');
            else
                $User = $_SESSION['Account']['_id'];
        }

        if( empty($PW) )
        {
            if( empty($_SESSION['Account']['Password']) )
                throw new Exception('AAPI PW not present in session - must specify.');
            else
                $PW = $_SESSION['Account']['Password'];
        }

        $this->la = $la;

        $this->CH = curl_init();

        curl_setopt($this->CH,CURLOPT_RETURNTRANSFER,TRUE);
        //curl_setopt($this->CH,CURLOPT_HEADER,TRUE);
        curl_setopt($this->CH,CURLINFO_HEADER_OUT,TRUE);

        // why doesn't verbose ever work in any permutation?
        // curl_setopt($this->CH,CURLOPT_VERBOSE,TRUE);
        // curl_setopt($this->CH,CURLOPT_STDERR,$this->Log);
        // $this->Log = fopen('php://memory','rw');

        // curl_setopt($this->CH,CURLOPT_SSL_VERIFYPEER,FALSE);
        // curl_setopt($this->CH,CURLOPT_SSL_VERIFYHOST,0);
        // curl_setopt($this->CH,CURLOPT_SSLCERT,fw('page')->Cert);
        // curl_setopt($this->CH,CURLOPT_SSLCERTPASSWD,fw('page')->Passwd);
        // curl_setopt($this->CH,CURLOPT_CERTINFO,TRUE);

        curl_setopt($this->CH,CURLOPT_HTTPHEADER,array("X-ASMBLR-USER: {$User}",
                                                         "X-ASMBLR-PW: {$PW}",
                                                              'Expect:'));
    }

    public function GET( $Routine,$Resource = NULL,$QueryString = array() )
    {
        curl_setopt($this->CH,CURLOPT_HTTPGET,TRUE);
        curl_setopt($this->CH,CURLOPT_URL,$this->la->Link($Routine,array(">{$Resource}")+$QueryString));

        $R = $this->Exec();
        if( empty($R) )
        {
            \fw\Log::Sys($Resource);
            \fw\Log::Sys($this->GetInfo());
        }
        else
            return $R;
    }

    public function POST( $Routine,$Payload = NULL )
    {
        curl_setopt($this->CH,CURLOPT_POST,TRUE);
        curl_setopt($this->CH,CURLOPT_URL,$this->la->Link($Routine));
        curl_setopt($this->CH,CURLOPT_POSTFIELDS,$Payload);

        $R = $this->Exec();
        if( empty($R) )
        {
            \fw\Log::Sys($Payload);
            \fw\Log::Sys($this->GetInfo());
        }
        else
            return $R;
    }

    public function GetInfo()
    {
        return curl_getinfo($this->CH);
    }

    // NULL is returned if a 400 bad request is encountered or a connection error happens
    // in which case GetInfo() and $ConnectionError can be examined
    // the response array is returned if received, which still should be checked by the caller
    protected function Exec()
    {
        $this->Response = curl_exec($this->CH);
        $this->ConnectionError = curl_error($this->CH);

        if( empty($this->ConnectionError) )
        {
            // sort of presumes a 400 bad request
            if( empty($this->Response) )
            {
                return NULL;
            }
            else
            {
                return json_decode($this->Response,TRUE);
            }
        }
        else
        {
            return NULL;
        }
    }
}
