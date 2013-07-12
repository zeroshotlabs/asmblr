<?php
namespace asm;

// asmblr specific validators
abstract class v extends \fw\enUS
{
    public static function Name( $N,&$H )
    {
        if( static::Text($N,$H) && strlen($H[$N]) < 50 )
            return TRUE;
        else
            return FALSE;
    }

    public static function Domain( $N,&$H )
    {
        if( static::Text($N,$H) )
        {
            $H[$N] = strtolower(trim(trim($H[$N],'.')));
            return TRUE;
        }
        else
            return FALSE;
    }

    public static function Path( $N,&$H )
    {
        if( static::Text($N,$H) && ($P = \fw\Path::Init($H[$N])) )
        {
            $H[$N] = \fw\Path::ToString($P);
            return TRUE;
        }
        else
            return FALSE;
    }

    public static function URL( $N,&$H )
    {
        // allow to clear URL (BaseURL)
        if( empty($H[$N]) )
        {
            $H[$N] = '';
            return TRUE;
        }
        else if( static::Text($N,$H) && ($U = \fw\URL::Init($H[$N])) )
        {
            $H[$N] = \fw\URL::ToString($U);
            return TRUE;
        }
        else
            return FALSE;
    }

    // this could be validated better - or really only at all since Init() assumes valid data
    // also this used to init a routine - that stuff should really done in the Page Struct and SetRoutine
    // sort of similar for the cleanup done by URL and Path - though we just it manually in things like Page::FromPHP
    public static function Routine( $N,&$H )
    {
        // allow to clear Routine
        if( empty($H[$N]) )
        {
            $H[$N] = array();
            return TRUE;
        }

        if( static::Textarea($N,$H) )
        {
            return TRUE;
//             try
//             {
//                 $H[$N] = \fw\Routine::Init($H[$N]);
//                 if( $H[$N]['Type'] !== 'Inline' )
//                     return FALSE;
//                 else
//                     return TRUE;
//             }
//             catch( \Exception $E )
//             {
//                 return FALSE;
//             }
        }
        else
            return FALSE;
    }

    // ditto
    public static function Body( $N,&$H )
    {
        if( empty($H[$N]) )
        {
            $H[$N] = '';
            return TRUE;
        }

        // really need a separate validator - maybe a binary/non?
        // return static::Textarea($N,$H);
        return TRUE;
    }


    public static function ContentType( $N,&$H )
    {
        if( empty($H[$N]) )
        {
            $H[$N] = '';
            return TRUE;
        }

        $T = \fw\HTTP::ResolveContentType($H[$N],TRUE);

        if( !empty($T) )
        {
            $H[$N] = $T;
            return TRUE;
        }
        else
            return FALSE;
    }

    // a directive is three part array, Name, Key, Value, contained directly in $_POST
    // for Name should be checking that value is known per REST::util_dir_names
    // since this works on the full $_POST and we then save to mongo, it should probably
    // screen-out any other variables that might be set
    public static function Directive( &$H )
    {
        if( !static::Text('Name',$H) || !static::Text('Key',$H) || !static::Textarea('Value',$H) )
            return FALSE;

        if( !empty($H['D_id']) && !static::_id('D_id',$H) )
            return FALSE;

        return TRUE;
    }

    // basically the same as Directives but simply K=>V each scalars
    public static function Config( $N,&$H )
    {
        // allow to clear Configs
        if( empty($H[$N]) )
            return TRUE;

        $T = json_decode($H[$N]);

        if( empty($T) )
            return FALSE;

        foreach( $T as $K => $C )
        {
            if( is_scalar($C) === FALSE )
                return FALSE;
        }

        $H[$N] = $T;
        return TRUE;
    }

    public static function _id( $N,&$H )
    {
        if( static::Text($N,$H) && strlen($H[$N]) === 24 )
            return TRUE;
        else
            return FALSE;
    }
}


// asmblr REST specific validators - always uses $_POST and has
// default names for each type
abstract class rv
{
    public static function Site_id( $N = 'Site_id' )
    {
        return v::_id($N,$_POST);
    }
    public static function Page_id( $N = 'Page_id' )
    {
        return v::_id($N,$_POST);
    }
    public static function Template_id( $N = 'Template_id' )
    {
        return v::_id($N,$_POST);
    }
    public static function Content_id( $N = 'Content_id' )
    {
        return v::_id($N,$_POST);
    }
    public static function Email( $N = 'Email' )
    {
        return v::Email($N,$_POST);
    }
    public static function Password( $N = 'Password' )
    {
        return v::Password($N,$_POST);
    }
    public static function Name( $N = 'Name' )
    {
        return v::Name($N,$_POST);
    }
    public static function Domain( $N = 'Domain' )
    {
        return v::Domain($N,$_POST);
    }
    public static function Path( $N = 'Path' )
    {
        return v::Path($N,$_POST);
    }
    public static function Status( $N = 'Status' )
    {
        if( v::Name($N,$_POST) )
            return in_array($_POST[$N],asm()->SiteStatuses,TRUE);
    }
    public static function BaseURL( $N = 'BaseURL' )
    {
        return v::URL($N,$_POST);
    }
    public static function Routine( $N = 'Routine' )
    {
        return v::Routine($N,$_POST);
    }
    // validates full $_POST
    public static function Directive()
    {
        return v::Directive($_POST);
    }
    public static function Config( $N = 'Config' )
    {
        return v::Config($N,$_POST);
    }
    public static function Body( $N = 'Body' )
    {
        return v::Body($N,$_POST);
    }
    public static function ContentType( $N = 'Type' )
    {
        return v::ContentType($N,$_POST);
    }
}

