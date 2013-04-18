<?php
namespace asm;


abstract class REST extends \fw\RoutineSet
{
    public static $Response;


    public static function v1()
    {

    }

    protected static function Auth()
    {
        if( !v::Email('Email',$_POST) || !v::Password('Password',$_POST) )
        {
            \fw\HTTP::_400();
        }
        else
        {
            $A = fw('as')->Auth($_POST['Email'],$_POST['Password']);

            if( empty($A) )
                static::$Response = JSONResponse::Fail('Invalid credentials.');
            else
                static::$Response = JSONResponse::Ok($A);
        }
    }

    protected static function SetPassword()
    {
        if( !v::Password('Password',$_POST) )
        {
            \fw\HTTP::_400();
        }
        else
        {
            $R = fw('as')->SetPassword($_POST['Password'],$A);

            if( empty($A) )
                \fw\HTTP::_401();
            else
                static::$Response = JSONResponse::Ok($A);
        }
    }


}

