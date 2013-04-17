<?php

namespace asm;


abstract class enUS extends \fw\enUS
{
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

    // primary key from x-editable:  id_CurrentTS
    // returns PK as array or NULL
    public static function PK( $R )
    {
        if( static::Text('pk',$R) )
        {
            $PK = explode('_',$R['pk']);

            if( count($PK) === 2 && ctype_digit($PK[1]) )
            {
                $PK[1] = (int) $PK[1];
                return $PK;
            }
            else
                return NULL;
        }
        else
            return NULL;
    }
}



abstract class AsmForms extends \fw\USValidate
{
    public static function Token( $Label,&$Src )
    {
        return \fw\IsToken($Src[$Label]);
    }

//     public static function Fragment( \fw\ValidationReport $vr,&$Data )
//     {
//         $vr->Token = static::Token('Token',$Data);
//         $vr->FragmentBody = static::Textarea('FragmentBody',$Data,FALSE);
//     }

//     public static function FrameURL( $Label,&$Src )
//     {
//         if( static::Text($Label,$Src) )
//         {
//             $Src[$Label] = ltrim($Src[$Label],'/');
//             return TRUE;
//         }
//         else
//             return FALSE;
//     }

    public static function Site( \fw\ValidationReport $vr,&$Data )
    {
        $vr->Domain = static::Text('Domain',$Data);

        static::Directives('Directives',$Data,$vr);

        $vr->RoutineBody = static::RoutineBody('RoutineBody',$Data);
    }

    // should be combined with Site form stuff?  partially at least
    public static function Page( \fw\ValidationReport $vr,&$Data )
    {
        $vr->Name = static::Token('Name',$Data);
        $vr->Domain = static::Text('Domain',$Data);
        $vr->Path = static::Text('Path',$Data);

        static::Directives('Directives',$Data,$vr);

        $vr->RoutineBody = static::RoutineBody('RoutineBody',$Data);
    }

    public static function Directives( $Label,&$Data,\fw\ValidationReport $vr )
    {
        $NewD = $Results = array();
        if( isset($Data['Directives']) && is_array($Data['Directives']) )
        {
            foreach( $Data['Directives'] as $K => $D )
            {
                if( \fw\Struct::Get('Token',$D) === '' )
                    continue;
                try
                {
                    $T = \fw\Directive::Init($D);
                    $NewD[] = $D;
                    $vr->{"Directive{$K}"} = TRUE;
                }
                catch( \Exception $E )
                {
                    $vr->{"Directive{$K}"} = FALSE;
                }
            }
        }
        $Data['Directives'] = $NewD;
    }

    public static function Template( \fw\ValidationReport $vr,&$Data )
    {
        $vr->Name = static::Token('Name',$Data);
        $vr->ContentType = static::SingleSelect('ContentType',array('text/html','text/css','text/javascript'),$Data,FALSE);

        $vr->Body = static::Textarea('Body',$Data);
    }

    // need to catch if PHP or HTML is in - otherwise hard to find parse errors happen
    public static function RoutineBody( $Label,&$Src )
    {
        if( !static::Textarea($Label,$Src,FALSE) )
            return FALSE;

        try
        {
            $T = \fw\Routine::Init($Src[$Label]);
            return TRUE;
        }
        catch( \Exception $E )
        {
            return FALSE;
        }
    }

    public static function Media( \fw\ValidationReport $vr,&$Data )
    {
        $vr->Name = static::Token('Name',$Data);
        $vr->Domain = static::Text('Domain',$Data);
        $vr->Path = static::Text('Path',$Data);

        // if content is specified, we use it, otherwise it'll be detected
        // during the upload process, so we don't need to do anything here
        $vr->Caption = static::Textarea('Caption',$Data);

        // Body can be anything (binary, etc) so don't need to do anything
    }
}




