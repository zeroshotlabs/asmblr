<?php


// misc utility stuff - needs cleanup


/**
 * A JSON request.
 *
 * This reads stdin which is typically used by other RESTful APIs.
 *
 * To read a default request from jQuery/XHR, just use $_POST.
 */
abstract class JSONRequest extends \asm\Struct
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
            static::$JSON = json_decode(file_get_contents('php://input'),TRUE);

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
     * @param string $Msg Optional message.
     * @retval array Response.
     */
    public static function Ok( $Data,$Msg = '' )
    {
        $R = static::$Skel;
        $R['Method'] = \asm\Path::ToURLString(Request::Init()['Path']);
        $R['Data'] = $Data;
        $R['Msg'] = $Msg;
        return $R;
    }

    /**
     * Initialize a failed response.
     *
     * @param string $Msg Error message.
     * @param mixed $Data Optional payload.
     * @retval array Response.
     */
    public static function Fail( $Msg,$Data = '' )
    {
        $R = static::$Skel;
        $R['Method'] = \asm\Path::ToURLString(Request::Init()['Path']);
        $R['Status'] = FALSE;
        $R['Msg'] = $Msg;
        $R['Data'] = $Data;
        return $R;
    }
}



// Sets PageCount, Skip and Window in $page in addition to the parameters passed in
// This could/should be just a Template Routine
function PageThis( $PageSize,$RecordCount,$CurrentPage,$WindowSize,$page )
{
    $PageCount = ceil($RecordCount/$PageSize);

    if( $CurrentPage > 1 )
    {
        $page->Prev = $CurrentPage-1;
        $page->PrevDisabled = '';
    }
    else
    {
        $page->Prev = 1;
        $page->PrevDisabled = 'disabled';
    }

    if( $CurrentPage < $PageCount )
    {
        $page->Next = $CurrentPage+1;
        $page->NextDisabled = '';
    }
    else
    {
        $page->Next = $PageCount;
        $page->NextDisabled = 'disabled';
    }

    $page->Last = $PageCount;
    $page->First = 1;

    $WindowStart = $CurrentPage-$WindowSize;
    $WindowStart = $WindowStart<1?1:$WindowStart;

    $WindowEnd = $CurrentPage+$WindowSize;
    $WindowEnd = $WindowEnd>$PageCount?$PageCount:$WindowEnd;

    $Window = array();
    for( $i = $WindowStart; $i <= $WindowEnd; ++$i )
        $Window[] = $i;

    // parameters
    $page->PageSize = $PageSize;
    $page->RecordCount = $RecordCount;
    $page->CurrentPage = $CurrentPage;
    $page->WindowSize = $WindowSize;

    // calculated
    $page->PageCount = $PageCount;
    $page->Skip = $PageSize * ($CurrentPage-1);
    $page->Window = $Window;
}



