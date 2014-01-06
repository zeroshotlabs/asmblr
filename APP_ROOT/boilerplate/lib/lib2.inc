<?php

/**
 * An XHR request from jQuery.
 */
abstract class jQueryRequest extends \asm\Struct
{
    protected static $JSON = FALSE;

    /**
     * Initialize a request array.
     *
     * @retval array jQueryRequest Struct array.
     */
    public static function Init()
    {
        if( static::$JSON === FALSE )
        {
            // jQuery sends data as POST by default
            static::$JSON = $_POST;

            // For other cases, such as RESTful APIs, it may be useful to
            // read the raw request directly from stdin.
            // static::$JSON = json_decode(file_get_contents('php://input'),TRUE);
        }

        return static::$JSON;
    }
}


/**
 * A JSON response array.
 */
abstract class JSONResponse extends \asm\Struct
{
    protected static $Skel = array('Method'=>'','Status'=>TRUE,'Msg'=>'','Data'=>'');

    /**
     * Initialize a successful response.
     *
     * @param mixed $Data Payload to include in response.
     * @retval array Response.
     */
    public static function Ok( $Data )
    {
        $R = static::$Skel;
        $R['Method'] = \asm\Path::ToURLString(fw()->Request['Path']);
        $R['Data'] = $Data;
        return $R;
    }

    /**
     * Initialize a failed response.
     *
     * @param string $Msg Error message.
     * @retval array Response.
     */
    public static function Fail( $Msg )
    {
        $R = static::$Skel;
        $R['Method'] = \asm\Path::ToURLString(fw()->Request['Path']);
        $R['Status'] = FALSE;
        $R['Msg'] = $Msg;
        return $R;
    }
}


