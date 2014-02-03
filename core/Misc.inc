<?php
/**
 * @file Misc.inc Miscellaneous tools and helpers.
 * @author Stackware, LLC
 * @version 4.0
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * Send messages between Page requests.
 *
 * Messager allows a message to be set on one Page and read from another.
 *
 * Messages are persisted using sessions and, by default, are deleted
 * after they are read.
 *
 * All Messager objects will use the same $_SESSION element and thus
 * share a common message namespace.
 */
class Messager
{
    public static function Alert( $Msg,$Class = '' )
    {
        if( !empty($Msg) )
        {
            if( Request::Init()['IsCLI'] )
                return "\r\n{$Msg}\r\n";
            else
                return "\r\n<pre class=\"alert {$Class}\">{$Msg}</pre>\r\n";
        }
    }

    public static function AlertSuccess( $Msg )
    {
        return static::Alert($Msg,'alert-success');
    }
    public static function AlertError( $Msg )
    {
        return static::Alert($Msg,'alert-error');
    }
    public static function AlertInfo( $Msg )
    {
        return static::Alert($Msg,'alert-info');
    }


    /**
     * Create a Messager object.
     *
     * This will also initialize the Messager element of the $_SESSION
     * array if it doesn't already exist.
     */
    public function __construct()
    {
        if( isset($_SESSION['Messager']) === FALSE )
            $_SESSION['Messager'] = array();
    }

    /**
     * Store a persistent message.
     *
     * Messages will persist until they are read.  Messages
     * can be any PHP variable that can be stored in sessions.
     *
     * If $Label already exists, it is silently overwritten.
     *
     * @param string $Label A label for referencing the message.
     * @param mixed $Value The message.
     */
    public function __set( $Label,$Value )
    {
        $_SESSION['Messager'][$Label] = $Value;
    }

    /**
     * Delete a message.
     *
     * @param string $Label The label of the message to delete.
     * @retval boolean TRUE if the $Label was found and deleted.
     */
    public function __unset( $Label )
    {
        if( isset($_SESSION['Messager'][$Label]) )
        {
            unset($_SESSION['Messager'][$Label]);
            return TRUE;
        }
        else
            return FALSE;
    }

    /**
     * Determine whether a message has been stored.
     *
     * @param string $Label The label of the message to check.
     * @retval boolean TRUE if the $Label was found.
     */
    public function __isset( $Label )
    {
        return isset($_SESSION['Messager'][$Label]);
    }

    /**
     * Retrieve and delete a stored message.
     *
     * Once read, the message will be deleted.
     *
     * @param string $Label The label of the message to retrieve.
     * @retval mixed The contents of the message.
     * @retval string An empty string of the $Label wasn't found.
     *
     * @note This does not perform any escaping.
     */
    public function __get( $Label )
    {
        if( isset($_SESSION['Messager'][$Label]) )
        {
            $T = $_SESSION['Messager'][$Label];
            unset($_SESSION['Messager'][$Label]);
            return $T;
        }
        else
            return '';
    }

    public function Error( $Label )
    {
        return static::AlertError($this->__get($Label));
    }
    public function Success( $Label )
    {
        return static::AlertSuccess($this->__get($Label));
    }
    public function Info( $Label )
    {
        return static::AlertInfo($this->__get($Label));
    }

    /**
     * Retrieve a message without deleting it.
     *
     * Unlike Messager::__get(), this will not delete the message
     * after retrieval.
     *
     * @param string $Label The label of the message to retrieve.
     * @retval mixed The contents of the message.
     * @retval string An empty string of the $Label wasn't found.
     *
     * @note This does not perform any escaping.
     */
    public function Read( $Label )
    {
        if( isset($_SESSION['Messager'][$Label]) )
            return $_SESSION['Messager'][$Label];
        else
            return '';
    }
}



/**
 * Track validation results.
 *
 * ValidationReport tracks and reports on individual element validation,
 * including whether the entire process is valid or not.
 *
 * ValidationReport can also return a $ValidValue or $InvalidValue
 * (typically strings or empty strings), depending on the
 * validity of individual checks.  When determining which value to
 * return, a valid check is considered !empty(), whereas an invalid
 * check is considered empty().
 *
 * ValidationReport is typically used for tracking the validity of
 * a form.
 */
class ValidationReport extends KeyValueSet
{
    /**
     * Overall validity of all individual checks.
     */
    protected $Valid = TRUE;

    /**
     * Default value to return when a check is invalid (empty()).
     */
    protected $InvalidValue;

    /**
     * Default value to return when a check is valid (!empty()).
     */
    protected $ValidValue;


    /**
     * Create a ValidationReport object.
     *
     * Default values for $InvalidValue and $ValidValue can be set.  While
     * any value could be set, typically a string or an empty string is used.
     *
     * @param mixed $InvalidValue Default value to return if a check is invalid (empty()).
     * @param mixed $ValidValue Default value to return if a check is valid (!empty()).
     */
    public function __construct( $InvalidValue = FALSE,$ValidValue = '' )
    {
        $this->InvalidValue = $InvalidValue;
        $this->ValidValue = $ValidValue;
    }

    /**
     * Check whether an individual element or overall report is valid.
     *
     * @param string $Label The element to check, or empty for overall report validity.
     * @param mixed $InvalidValue Value to return if a check is invalid (empty()), overriding the object default.
     * @param mixed $ValidValue Value to return if a check is valid (!empty()), overriding the object default.
     * @retval boolean TRUE if the overall report is valid and no individual element was checked.
     * @retval mixed $InvalidValue or the object's default if the check is invalid (empty()).
     * @retval mixed $ValidValue or the object's default if the check is valid (!empty()).
     * @retval NULL The element was not found.
     *
     * @note If the object's default InvalidValue or ValidValue is set to NULL, the return value is indeterminate.
     * @note Overall report validity is TRUE only if every individual element is valid (!empty()).
     */
    public function __invoke( $Label = '',$InvalidValue = NULL,$ValidValue = NULL )
    {
        if( empty($Label) )
        {
            foreach( $this->KV as $V )
            {
                if( empty($V) )
                    return FALSE;
            }

            return TRUE;
        }
        else if( array_key_exists($Label,$this->KV) )
        {
            return empty($this->KV[$Label])?($InvalidValue===NULL?$this->InvalidValue:$InvalidValue):($ValidValue===NULL?$this->ValidValue:$ValidValue);
        }
        else
            return NULL;
    }

    /**
     * Get the validity value of an individual element as it was set.
     *
     * The return value of an element's validator (i.e., enUS::Text()) will be returned.
     *
     * @param string $Label The element to get the validity value of.
     * @retval mixed The element's validity value, as it was set.
     * @retval NULL The element was not found.
     */
    public function __get( $Label )
    {
        return isset($this->KV[$Label])?$this->KV[$Label]:NULL;
    }

    /**
     * Set the validity of an individual element.
     *
     * An existing element with the same Label is silently overwritten.
     *
     * @param string $Label The element to set the validity of.
     * @param mixed $Value The validity value of the element.
     *
     * @note While any value is accepted, typically only a boolean value should be set.  If
     *       other values are used - especially NULL - the behavior of the other functions and
     *       the validity of the overall report could become undefined.
     * @see ValidationReport::__invoke()
     */
    public function __set( $Label,$Value )
    {
        $this->KV[$Label] = $Value;
    }
}


/**
 * Math functions :)
 */
abstract class Math
{
    /**
     * Calculate a percentage to a given number of decimal places.
     *
     * @param scalar $Part A value that can be converted into a float.
     * @param scalar $Total A value that be converted into a float.
     * @param int $Places The number of decimal places to return.
     * @retval string The percentage with percent sign or n/a if divide by zero.
     */
    public static function Percentage( $Part,$Total,$Places = 2 )
    {
        settype($Part,'float');
        settype($Total,'float');

        if( $Total === 0.0 )
            return 'n/a';
        else
            return (round($Part/$Total,$Places)*100).'%';
    }
}

/**
 * Tools for generating database agnostic SQL strings.
 */
abstract class SQL
{
    /**
     * Convert an array of column names to a SQL string.
     *
     * ToColumnList() forms the list of returned columns in a SELECT.
     *
     * The value of a numerically indexed element is taken as a column name.
     *
     * An associative element is taken as a column name/column alias pair.
     *
     * Neither column names nor aliases are escaped or quoted in any way.
     *
     * @param array $Src The array to convert.
     * @retval string SQL string of column names/aliases.
     */
    public static function ToColumnList( $Src )
    {
        $SQL = '';
        $i = 0;
        foreach( $Src as $K => $V )
            $SQL .= ($i++>0?',':'').(is_string($K)?"$K AS $V":$V);

        return $SQL;
    }

    /**
     * Convert an array of column names and booleans to a SQL string.
     *
     * ToOrderBy() forms an ORDER BY clause.
     *
     * The value of each element is the direction of ordering and
     * expected to be boolean.  Boolean TRUE is taken as ascending (ASC)
     * and boolean FALSE is taken as descending (DESC).
     *
     * Column names are not escaped or quoted in any way.
     *
     * @param array $OrderBy An associative array of column names/booleans.
     * @retval string SQL string of column names/directions.
     *
     * @note Any non-string column name or non-boolean value is silently skipped.
     */
    public static function ToOrderBy( $OrderBy )
    {
        $SQL = '';
        $i = 0;
        foreach( $OrderBy as $K => $V )
        {
            if( is_string($K) && is_bool($V) )
            {
                $SQL .= ($i++>0?',':'')."$K ".($V?'ASC':'DESC');
            }
        }

        return $SQL;
    }
}


// helps with forming URLs and communicating GET/POST with an RESTish API
// methods needs to be configurable
// TODO: add response headers easily available, outside of using $Raw
class restr extends \asm\LinkSet
{
    protected $Methods = array();
    protected $AuxOptions;


    public function __construct( $BaseURL = '',$UserPass = array(),$Options = array() )
    {
        if( !empty($BaseURL) )
            $this->SetBaseURL($BaseURL);
        else if( !empty($this->BaseURL) )
            $this->SetBaseURL($this->BaseURL);
        else
            throw new Exception('No restr BaseURL defined.');

        $this->AuxOptions = $Options;
    }

    public function __invoke( $Name = NULL,$Set = array() )
    {
        if( isset($this->Methods[$Name]) )
        {
            $Base = $this->BaseURL;
            \asm\Path::Merge(\asm\Path::Init($this->Methods[$Name]),$Base['Path']);

            if( !empty($Set) )
                \asm\URL::Set($Set,$Base);

            return \asm\URL::ToString($Base);
        }
        else
            return 'not-found';
    }

    // assumes a JSON response
    public function GET( $Method,$QueryString = array(),$Path = '',$Raw = FALSE )
    {
        $D = array('http'=>array_merge($this->AuxOptions,array('method'=>'GET','follow_location'=>'5')));

        // extra options querystring/path not currently supported
        if( $Raw === TRUE )
        {
            $R = array('URL'=>$this($Method),'Context'=>$D,'ResponseHeaders'=>array(),'Response'=>'');
            $R['Response'] = file_get_contents($this($Method),FALSE,stream_context_create($D));
            $R['ResponseHeaders'] = $http_response_header;
            return $R;
        }
        else
            return json_decode(file_get_contents($this($Method),FALSE,stream_context_create($D)),TRUE);
    }

    // assumes a JSON reponse
    public function POST( $Method,$Payload = array(),$Raw = FALSE )
    {
        $D = array('http'=>array_merge($this->AuxOptions,array('method'=>'POST')));

        // TODO: is this Content-Type correct?
        if( !empty($D['http']['header']) )
            $D['http']['header'] =  trim($D['http']['header'])."\r\nContent-Type: application/x-www-form-urlencoded";
        else
            $D['http']['header'] = 'Content-type: application/x-www-form-urlencoded';

        if( !empty($Payload) )
            $D['http']['content'] = http_build_query($Payload);

        if( $Raw === TRUE )
        {
            $R = array('URL'=>$this($Method),'Context'=>$D,'ResponseHeaders'=>array(),'Response'=>'');
            $R['Response'] = file_get_contents($this($Method),FALSE,stream_context_create($D));
            $R['ResponseHeaders'] = $http_response_header;
            return $R;
        }
        else
            return json_decode(file_get_contents($this($Method),FALSE,stream_context_create($D)),TRUE);
    }

    /**
     * Helper method for parsing $http_response_header (PHP streams) into an
     * associative array.  The full HTTP response header will be available in
     * the 'HTTP' element.
     *
     * @param array $Raw $http_response_header from a PHP streams call.
     * @retval array Associative array of headers/values.
     *
     * @note Multiple headers of the same name will overwrite.
     */
    public static function ParseHeaders( $Raw )
    {
        $Headers = array();
        foreach( $Raw as $V )
        {
            if( strpos($V,'HTTP/') === 0 )
            {
                $Headers['HTTP'] = $V;
            }
            else
            {
                $K = explode(':',$V);
                $Headers[trim($K[0])] = trim($K[1]);
            }
        }

        return $Headers;
    }
}


