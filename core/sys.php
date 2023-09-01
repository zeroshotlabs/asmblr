<?php declare(strict_types=1);
/**
 * @file sys.php Extension loading, debugging and error handling (exceptions!!?!?!).
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm\sys;


/**
 * Load an extension that's bundled with asmblr under the ext/ directory.
 *
 * @note since the old extensions are broken anyway this only supports
 * loading ext-directory/load.inc - needs updating.
 * 
 * @c $ExtLoader is tried in the following way, relative to ext/:
 *   - literally as a filename
 *   - as a directory with a Load.inc
 * 
 * It is case-sensitive.
 *
 * @param string $ExtLoader The extension's filename or directory.
 * @throws Exception Extension '$ExtLoader' not found.
 *
 * @note This uses require() so pay attention.
 * @todo Improve path handling - ASM_EXT_ROOT is hardwired currently.
 */
function load_ext( $ext )
{
    if( is_file(ASM_EXT_ROOT.$ext.DIRECTORY_SEPARATOR."load.inc") )
        require(ASM_EXT_ROOT.$ext.DIRECTORY_SEPARATOR."load.inc");
    else
        throw new Exception("Extension '$ext' not found.");
}


/**
 * Shorthand for Log::Log().
 *
 * @see Log::Log()
 * @todo This could probably be improved according to named params.
 * @todo This can probably be replaced by _stdo/_stde functions and improved logger.
 */
function llog( $Msg,$Level = 'LOG',$Backtrace = NULL,$Context = NULL )
{
    foreach( func_get_args() as $V )
        \asm\Log::Log($V,$Level,$Backtrace,$Context);
}



//basically debug/log - needs work
// @todo this needs to be updated to new logging stuff
// output to console
// this should probably go in Base or somewhere with the debug stuff/llog
// should add coloring/error message/etc
function _stdo( $msg,$pre_lines = 1,$post_lines = 2 )
{
    $pre_lines = str_repeat(PHP_EOL,$pre_lines);
    $post_lines = str_repeat(PHP_EOL,$post_lines);

    fwrite($GLOBALS['STDOUT']??STDOUT,"{$pre_lines}{$msg}{$post_lines}");
}

function _stde( $msg,$pre_lines = 1,$post_lines = 2 )
{
    $pre_lines = str_repeat(PHP_EOL,$pre_lines);
    $post_lines = str_repeat(PHP_EOL,$post_lines);

    // @note see promptd
    error_log("{$pre_lines}{$msg}{$post_lines}");
}


/**
 * Allow internal debugging of the implementing class.
 */
interface Debuggable
{
    /**
     * Turn on debugging for the Wire()'d object.
     *
     * Debugging behavior is determined only by the object itself and should
     * be on when a configured DebugToken is present in $_SERVER.
     */
    public function DebugOn( $Label = NULL );

    /**
     * Turn off debugging for the Wire()'d object.
    */
    public function DebugOff();

}

/**
 * Default debugging methods for the Debuggable interface.
 */
trait Debugged
{
    /**
     * Token for toggling debugging for the object.
     * It can be checked using @c isset($_SERVER[$this->DebugToken])
     */
    protected $DebugToken;


    /**
     * Enable debugging.
     *
     * Debugging is controlled by an element of name @c Debugged::$DebugToken
     * in the @c $_SERVER super-global.
     *
     * @param NULL $Label Log messages will be labeled with the class name.
     * @param string $Label Custom label for log messages.
     *
     * @todo $Label is misused in PageSet/TemplateSet as a way to trigger output of debug info - needs clean-up.
     */
    public function DebugOn( $Label = NULL )
    {
        if( empty($Label) )
            $this->DebugToken = get_class($this);
        else
            $this->DebugToken = $Label;

        $_SERVER[$this->DebugToken] = TRUE;
    }

    /**
     * Disable debugging.
     */
    public function DebugOff()
    {
        if( !empty($this->DebugToken) )
            unset($_SERVER[$this->DebugToken]);
    }

    /**
     * Determine whether debugging is enabled.
     *
     * @retval boolean TRUE if debugging is enabled.
     */
    public function IsDebug()
    {
        return !empty($_SERVER[$this->DebugToken]);
    }
}


/**
 * Log messages to a destination or notify someone.
 *
 * Log can send messages to the following destinations:
 *  - Sys: web server (SAPI) logging mechanism.
 *  - Email: to the SysOp or custom address.
 *  - Wildfire: FirePHP HTTP header protocol for browser console debugging.
 *  - ChromePHP: HTTP header protocol for browser console debugging.
 *
 * The Sys and Email mechanisms are considered private logging.  If the
 * @c LogPublic variable is TRUE, Wildfire/ChromePHP logging will be
 * automatically detected and used if available, or fallback to Sys.
 *
 * The following levels are available: 'LOG','INFO','WARN','ERROR'.
 * The effect of each depends on the destination.
 */
abstract class Log
{
    /**
     * Log a message using an optimal destination.
     *
     * Log::Log() will deliver a message to an automatically determined
     * mechanism, based on these criteria:
     *  - If IsCLI is TRUE, Log::Sys() is used.
     *  - If LogPublic is TRUE, detect FirePHP or ChromePHP plugin and output
     *    using Log::Wildfire() or Log::ChromePHP().
     *  - Fallback to Log::Sys().
     *
     * Logging should be funneled through this method to ensure log
     * configuration (App::$LogPublic) is correctly honored.
     *
     * The global function llog() is shorthand for this method, Log::Log(),
     * and is meant as a replacement for var_dump().
     *
     * @param string $Msg The message to log.
     * @param mixed $Msg An array or object to log - non-scalar output depends on destination.
     * @param string $Level The message severity (LOG, INFO, WARN, ERROR).
     * @param string $Backtrace A string indicating where the message originated.
     * @param array $Backtrace A backtrace array (array of strings).
     * @param array $Context Local variables from the scope in which the message originated.
     *
     * @note This expects that the error handlers and Request are already initialized.
     * @todo Add Email support.
     * @todo this needs update - $asmdpp is old
     *
     * @note Chrome PHP doesn't announce itself so we're not supporting it anymore
     */
    public static function Log( $Msg,$Level = 'LOG',$Backtrace = NULL,$Context = NULL )
    {
        global $asmdpp;

        $IsCLI = $asmdpp->Request['IsCLI'];

        // if this isn't even set we likely have a problem very early on so just dump the message and hope
        if( !isset($asmdpp->Config['LogPublic']) )
            exit("CRITICAL: $Msg");

        if( count(debug_backtrace()) > 100 )
        {
            trigger_error("Sorry, logging is apparently stuck in recursion (100+ deep) - bye.\r\n\r\n".Debug::Dump(Debug::Backtrace()),'ERROR',debug_backtrace(),NULL);
            exit;
        }

        // If the first backtrace line is a string, we take it as where an error occurred.
        if( is_array($Backtrace) && Is::String(0,$Backtrace) )
            $Msg .= " {$Backtrace[0]}";

        $Context = (array) $Context;

        // Always add the requested URL as part of the context
        $Context['Request'] = URL::ToString($asmapp->Request);

        // CLI - use Log::Sys() - other option would be Email
        if( $IsCLI === TRUE )
        {
            Log::Sys($Msg,$Level,$Backtrace,$Context);
        }
        else
        {
            // detect FirePHP or ChromePHP or fallback to Log::Sys()
            // @todo update with https://craig.is/writing/chrome-logger   https://github.com/ccampbell/chromephp
            if( $asmapp->Config['LogPublic'] === TRUE )
            {
                if( Request::IsFirePHP() )
                    Log::Wildfire($Msg,$Level,$Backtrace,$Context);
//                else if( Request::IsChromePHP() )
//                    Log::ChromePHP($Msg,$Level,$Backtrace,$Context);
                else
                    Log::Sys($Msg,$Level,$Backtrace,$Context);
            }
            // log internally using Log::Sys() - other option would be Email
            else
            {
                Log::Sys($Msg,$Level,$Backtrace,$Context);
            }
        }
    }

    /**
     * Log a message to the SAPI's default logging mechanism.
     *
     * This is a private logging destination.
     *
     * @param string $Msg The message to log.
     * @param mixed $Msg An array or object to log - non-scalars will be dumped as a string.
     * @param string $Level The message severity (LOG, INFO, WARN, ERROR).
     * @param string $Backtrace A string indicating where the message originated.
     * @param array $Backtrace A backtrace array (array of strings).
     * @param array $Context Local variables from the scope in which the message originated.
     *
     * @note This uses error_log() 0 and 4 on Windows and Linux, respectively.
     * @todo Context and Backtrace are currently ignored.
     * @todo Do we want to use syslog() instead (for GAE PHP)?
     * @todo Log entry length limits are a drag - how to force multiple seperate entries
     *       in the logs (seperate timestamps)?
     */
    public static function Sys( $Msg,$Level = 'LOG',$Backtrace = NULL,$Context = NULL )
    {
        // ini_set('log_errors_max_len', 0);

        if( !is_string($Msg) )
            $Msg = Debug::Dump($Msg);

        // CLI - always direct
        if( Request::Init()['IsCLI'] === TRUE )
        {
            $t = ini_get('error_log');

            // no log set - output using SAPI log
            if( empty($t) )
                error_log("\r\n".str_replace(array("\r\n","\n"),"\r\n",$Msg),4);
            // use log file
            else
                error_log("\r\n".str_replace(array("\r\n","\n"),"\r\n",$Msg),0);
        }
        // We're on Windows so do it all in one shot and change line endings - assumes IIS - goes to php.log file
        else if( App::IsWindows() === TRUE )
        {
            error_log(str_replace(array("\r\n","\n"),"\r\n",$Msg),0);
/*
            if( !empty($Context) )
                error_log('CONTEXT: '.Debug::Dump($Context),0);
*/

        }
        // Assume Linux and log line-by-line for easier reading - lines less than 300 so they don't get clipped
        // @todo Would love to be able to flush these before the end of the request?
        else
        {
            foreach( explode("\n",trim($Msg) ) as $Line )
            {
                // ,3,"/tmp/asmblr.log"
                // @todo still not right - recompile to extend the log size? how to do multiple seperate entries?
                error_log($Line.PHP_EOL);

                // $l = strlen($Line);
                // $i = 0;
                // do {
                //     error_log("\n H".substr($Line,$i,($i+=300))."\n",4);
                //     $l -= 300;
                // } while( $l >= 300 );
            }

        }
    }

    /**
     * Send a notification message by sending an email.
     *
     * By default the email is sent to the configured value of $SysOp from the manifest.
     *
     * If sending fails, the messages is logged to Log::Sys().
     *
     * This is considered a private logging destination.
     *
     * @param string $Msg The message to log.
     * @param mixed $Msg An array or object to log - non-scalars will be dumped as a string.
     * @param string $Level The message severity (LOG, INFO, WARN, ERROR).
     * @param string $Backtrace A string indicating where the message originated.
     * @param array $Backtrace A backtrace array (array of strings).
     * @param array $Context Local variables from the scope in which the message originated.
     * @param string $Email Optional email address to send to instead of App::$SysOp.
     *
     * @todo $Context is ignored (and in other methods too).
     */
    public static function Email( $Msg,$Level = 'LOG',$Backtrace = NULL,$Context = NULL,$Email = NULL )
    {
        global $asmapp;

        if( !empty($Email) )
            $To = $Email;
        else
            $To = $asmapp->Config['SysOp'];

        $From = $asmapp->Config['SysOp'];
        $FromDisplay = "asmblr <{$From}>";

        if( !is_string($Msg) )
            $Msg = Debug::Dump($Msg);

        $Msg .= Debug::Dump($Backtrace);

        if( mail($To,"ASMBLR {$Level}",$Msg,"Reply-To: {$From}\nFrom: {$FromDisplay}",'-f'.$From) !== TRUE )
        {
            $Msg = "Logging error because mail() failed to '$To' \r\n{$Msg}";
            static::Log($Msg,$Level,$Backtrace,$Context);
        }
    }

    /**
     * Log a message to the browser's console using FirePHP (Wildfire).
     *
     * @param string $Msg The message to log.
     * @param mixed $Msg An array or object to log - non-scalars will be dumped to string.
     * @param string $Level The message severity (LOG, INFO, WARN, ERROR).
     * @param string $Backtrace A string indicating where the message originated.
     * @param array $Backtrace A backtrace array (array of strings).
     * @param array $Context Local variables from the scope in which the message originated.
     */
    public static function Wildfire( $Msg,$Level = 'LOG',$Backtrace = NULL,$Context = NULL )
    {
        static $Rows = array();

        header('X-Wf-Protocol-1: http://meta.wildfirehq.org/Protocol/JsonStream/0.2');
        header('X-Wf-1-Plugin-1: http://meta.firephp.org/Wildfire/Plugin/FirePHP/Library-FirePHPCore/0.3');
        header('X-Wf-1-Structure-1: http://meta.firephp.org/Wildfire/Structure/FirePHP/FirebugConsole/0.1');

        $Context = $Context['Request'];

        $Meta = array('Type'=>$Level,'Label'=>'FW');
        $Body = array('Msg'=>$Msg,'Context'=>$Context,'Backtrace'=>$Backtrace);

        $T = array($Meta,$Body);
        Struct::ToUTF8($T);
        // see ChromePHP note
        $Rows[] = @json_encode($T);

        foreach( $Rows as $K => $R )
        {
            header('X-Wf-1-1-1-'.($K+1).': '.strlen($R)."|{$R}|");
        }
    }

    /**
     * Log a message to the browser's console using ChromePHP.
     *
     * @param string $Msg The message to log.
     * @param mixed $Msg An array or object to log - non-scalars will be dumped as collapsible objects.
     * @param string $Level The message severity (LOG, INFO, WARN, ERROR).
     * @param string $Backtrace A string indicating where the message originated.
     * @param array $Backtrace A backtrace array (array of strings).
     * @param array $Context Local variables from the scope in which the message originated.
     *
     * @note $Context is killed except for Request.
     */
    public static function ChromePHP( $Msg,$Level = 'LOG',$Backtrace = NULL,$Context = NULL )
    {
        static $H = array('version'=>'0.2','columns'=>array('label','log','backtrace','type'),'rows'=>array());

        // context can contain a lot of strange data so we kill it
        $Context = $Context['Request'];
        $H['rows'][] = array("FW",array('Msg'=>$Msg,'Context'=>$Context,'Backtrace'=>$Backtrace),'',strtolower($Level));

        Struct::ToUTF8($H);

        // if json_encode() finds something it doesn't understand (like a variable containing binary image
        // string or a PHP resource) it's error will cause very strange recursion - need to further test with xdebug
        // we could also surpress errors here but for now we're blanking $Context
        $T = json_encode($H);

        // this often requires output_buffering = on since most output will be on it's way before this is called
        header('X-ChromePhp-Data: '.base64_encode(mb_convert_encoding($T,'UTF-8')));
    }
}

/**
 * Helper methods for working with and outputting backtraces and variables dumps.
 */
abstract class Debug
{
    /**
     * Return a backtrace as a smaller, friendlier, array.
     *
     * @param array $Ignores Optional set of function calls to ignore from the backtrace - Backtrace and ErrorHandler
     *                       are always included.
     * @retval array The backtrace array.
     *
     * @todo Determine original call details for originators such as ReMap(), rendering/eval()/__call, Directives, if possible.
     */
    public static function Backtrace( $Ignores = array() )
    {
        $BT = array();
        $Ignores = array_merge($Ignores,array('Backtrace','ErrorHandler'));
        foreach( debug_backtrace() as $K => $V )
        {
            $T = array('Class'=>isset($V['class'])?$V['class']:'');

            $T['Function'] = isset($V['function'])?$V['function']:'';
            if( in_array($T['Function'],(array)$Ignores) )
                continue;

            $T['Type'] = isset($V['type'])?$V['type']:'';

            // this has to be done like this otherwise we reference the actual object
//            $T['Args'] = isset($V['args'])?$V['args']:array();
            $T['Args'] = array();
            if( isset($V['args']) )
            {
                foreach( $V['args'] as $K2 => $V2 )
                    $T['Args'][$K2] = is_object($V2)?get_class($V2).' object':$V2;
            }

            $T['File'] = Struct::Get('file',$V);
            $T['Line'] = Struct::Get('line',$V);
            $T['Basename'] = basename($T['File']);

            $BT[] = $T;
        }

        return $BT;
    }

    /**
     * Convert a backtrace into an array of compact strings.
     *
     * @param array $BT Backtrace array.
     * @param array $FuncFilter Array of functions to include - all others will be skipped.
     * @retval string The bactrace string.
     */
    public static function BT2Str( $BT,$FuncFilter = array() )
    {
        $Buf = array();
        foreach( $BT as $K => $V )
        {
            if( !empty($FuncFilter) )
            {
                if( in_array($V['Function'],(array)$FuncFilter) === FALSE )
                    continue;

                $Buf[] = (strpos($V['Basename'],'eval()\'d')!==FALSE?"eval()({$V['Line']})":"{$V['Basename']}({$V['Line']})");
            }
            else
            {
                $Buf[] = "{$V['Class']}{$V['Type']}{$V['Function']} from "
                        .(strpos($V['Basename'],'eval()\'d')!==FALSE?"eval()({$V['Line']})":"{$V['Basename']}({$V['Line']})");
            }
        }

        return $Buf;
    }

    /**
     * Return a string that's a human-friendly dump of a variable.
     */
    public static function Dump( $Mixed,$Label = NULL )
    {
        if( $Label !== NULL )
            $Buf = "\r\n\r\n---SHOWING LABEL '$Label' OF TYPE ".gettype($Mixed);
        else
            $Buf = "\r\n\r\n---SHOW VARIABLE OF TYPE ".gettype($Mixed);

        ob_start();
        var_dump($Mixed);
        return $Buf."\r\n".ob_get_clean();
    }
}




// /**
//  * A Struct manages arrays that have an overall consistent structure.
//  *
//  * This is a base class that's meant to be extended for application specific data structures.
//  *
//  * Structs support both associative and integer indexed arrays, or even a mixed
//  * array.  However, mixing the two in a single array is not recommended, and can cause
//  * unpredictable results.
//  *
//  * For the methods with a RefPoint parameter, these rules are used:
//  *
//  *  - When RefPoint is an integer, working with an integer array is assumed and keys are not maintained.
//  *  - When RefPoint is a string, working with an associative array is assumed and keys are maintained.
//  *
//  * All methods that insert a new Element will attempt to insert it as a whole - only one
//  * new element will be created.
//  *
//  * By convention, the general structure of a Struct managed array is defined in the
//  * $Skel property.  A new Struct array is created by an Init() method and returned.
//  *
//  * A general way to think of Structs is as the minimal amount of elements and their
//  * expected values/types that are needed for a purpose.  The array may contain more
//  * elements.
//  *
//  * @see Is for methods that operate on elements of generic arrays.
//  * @deprecated replaced by DAO
//  */
// abstract class Struct
// {
//     /**
//      * Default structure for a Struct.
//      */
//     protected static $Skel = array();


//     /**
//      * Determine whether the array has a known structure.
//      *
//      * This compares the keys of $Arr to that of Struct::$Skel.
//      *
//      * If a more complete comparision, including type checking, is needed,
//      * see Is::Equal().
//      *
//      * @param array $Arr The array to check.
//      * @retval boolean TRUE if the keys are known.
//      */
//     public static function IsA( $Arr )
//     {
//         return (array_keys($Arr) === array_keys(static::$Skel));
//     }

//     /**
//      * Append a new element to the array.
//      *
//      * @param mixed $Element Element to append.
//      * @param array $Subject The array to append to.
//      * @throws Exception Element with key already exists.
//      * @throws Exception Invalid Element.
//      * @retval int The position, counting from 0, at which the element was appended.
//      */
//     public static function Append( $Element,&$Subject )
//     {
//         if( is_array($Element) === FALSE )
//         {
//             $Subject[] = $Element;
//             return count($Subject)-1;
//         }
//         else if( count($Element) === 1 )
//         {
//             if( isset($Subject[key($Element)]) === FALSE || is_int(key($Element)) === TRUE )
//             {
//                 $Subject = array_merge($Subject,$Element);
//                 return count($Subject)-1;
//             }
//             else
//                 throw new Exception('Element with key '.key($Element).' already exists.');
//         }
//         else
//             throw new Exception('Invalid Element.');
//     }

//     /**
//      * Prepend a new element to the array.
//      *
//      * @param mixed $Element Element to prepend.
//      * @param array $Subject The array to prepend to.
//      * @throws Exception Element with key already exists.
//      * @throws Exception Invalid Element.
//      * @retval int 0
//      */
//     public static function Prepend( $Element,&$Subject )
//     {
//         if( is_array($Element) === FALSE )
//         {
//             array_splice($Subject,0,0,$Element);
//             return 0;
//         }
//         else if( count($Element) === 1 )
//         {
//             if( isset($Subject[key($Element)]) === FALSE || is_int(key($Element)) === TRUE )
//             {
//                 //array_splice($Subject,0,0,$Element);
//                 $Subject = array_merge($Element,$Subject);
//                 return 0;
//             }
//             else
//                 throw new Exception('Element with key '.key($Element).' already exists.');
//         }
//         else
//             throw new Exception('Invalid Element.');
//     }

//     /**
//      * Insert a new element before another element.
//      *
//      * Element is inserted as a whole, regardless of it's keys, if any.
//      *
//      * @param mixed $Element Element to insert.
//      * @param int|string $RefPoint Reference point of insertion as either an associative or numeric key.
//      * @param array &$Subject The array to insert into.
//      * @throws Exception Element with key already exists.
//      * @throws Exception Invalid RefPoint or Element.
//      * @retval int The position, counting from 0, at which insertion occurred.
//      * @retval NULL RefPoint was not found and nothing was inserted.
//      *
//      * @note This probably isn't bullet proof.
//      */
//     public static function InsertBefore( $Element,$RefPoint,&$Subject )
//     {
//         if( is_int($RefPoint) === TRUE )
//         {
//             if( isset($Subject[$RefPoint]) === TRUE )
//             {
//                 array_splice($Subject,$RefPoint,0,is_array($Element)===TRUE?array($Element):$Element);
//                 return $RefPoint;
//             }
//             else
//                 return NULL;
//         }
//         else if( is_string($RefPoint) === TRUE && is_array($Element) === TRUE )
//             // && count($Element) === 1
//         {
//             if( isset($Subject[key($Element)]) === FALSE )
//             {
//                 if( ($RefPoint = static::KeyPosition($RefPoint,$Subject)) === NULL )
//                     return NULL;
//                 $Subject = array_merge(array_splice($Subject,0,$RefPoint),$Element,$Subject);
//                 return $RefPoint;
//             }
//             else
//                 throw new Exception('Element with key '.key($Element).' already exists.');
//         }
//         else
//             throw new Exception('Invalid RefPoint or Element.');
//     }

//     /**
//      * Insert a new element after another element.
//      *
//      * Element is inserted as a whole, regardless of it's keys, if any.  If it's an array, it can
//      * have only a single key/value pair (unlike InsertBefore).
//      *
//      * @param mixed $Element Element to insert.
//      * @param int|string $RefPoint Reference point of insertion as either an associative or numeric key.
//      * @param array &$Subject The array to insert into.
//      * @throws Exception Element with key already exists.
//      * @throws Exception Invalid RefPoint or Element.
//      * @retval int The position, counting from 0, at which insertion occurred.
//      * @retval NULL RefPoint was not found and nothing was inserted.
//      *
//      * @note This probably isn't bullet proof.
//      */
//     public static function InsertAfter( $Element,$RefPoint,&$Subject )
//     {
//         if( is_int($RefPoint) === TRUE )
//         {
//             if( isset($Subject[$RefPoint]) === TRUE )
//             {
//                 array_splice($Subject,++$RefPoint,0,is_array($Element)===TRUE?array($Element):$Element);
//                 return $RefPoint;
//             }
//             else
//                 return NULL;
//         }
//         else if( is_string($RefPoint) === TRUE && is_array($Element) === TRUE && count($Element) === 1 )
//         {
//             if( isset($Subject[key($Element)]) === FALSE )
//             {
//                 if( ($RefPoint = static::KeyPosition($RefPoint,$Subject)) === NULL )
//                     return NULL;
//                 $Subject = array_merge(array_splice($Subject,0,++$RefPoint),$Element,$Subject);
//                 return $RefPoint;
//             }
//             else
//                 throw new Exception('Element with key '.key($Element).' already exists.');
//         }
//         else
//             throw new Exception('Invalid RefPoint or Element.');
//     }

//     /**
//      * Remove an element from an array.
//      *
//      * If $Needle is an integer, the remaining keys will be reindexed.  In an associative,
//      * $Needle indicates the position of the element to remove, not it's actual key.
//      *
//      * @param int|string $Needle Key of the element to remove.
//      * @param array &$Haystack The array to remove the element from.
//      * @retval boolean TRUE if $Needle was found and it's element removed.
//      */
//     public static function Del( $Needle,&$Haystack )
//     {
//         if( isset($Haystack[$Needle]) === TRUE )
//         {
//             if( is_int($Needle) === TRUE )
//                 array_splice($Haystack,$Needle,1);
//             else
//                 unset($Haystack[$Needle]);

//             return TRUE;
//         }
//         else
//             return FALSE;
//     }

//     /**
//      * Read or check the value of an element.
//      *
//      * If $Check is provided, strict type checking is used.
//      *
//      * @param int|string $Needle Key of the element to read.
//      * @param array $Haystack The array to read the element from.
//      * @param mixed $Check Optional value to check the element's value against.
//      * @retval mixed The element's value if no $Check was provided.
//      * @retval NULL $Needle was not found and no $Check was provided.
//      * @retval boolean TRUE if $Check was provided and the values matched.
//      */
//     public static function Get( $Needle,$Haystack,$Check = NULL )
//     {
//         if( isset($Haystack[$Needle]) === TRUE )
//             return ($Check===NULL)?$Haystack[$Needle]:($Haystack[$Needle]===$Check);
//         else
//             return NULL;
//     }

//     /**
//      * Convert the values of an array into a string.
//      *
//      * @param array $Subject The array to convert.
//      * @param string $Surround A string to surround each value with.
//      * @param string $Delim A string to seperate each value.
//      * @retval string The array values as a string.
//      * @retval NULL Subject was not an array.
//      *
//      * @note If $Surround is a single character and appears in a value, it is escaped with a backslash.
//      */
//     public static function Dissolve( $Subject,$Surround = '\'',$Delim = ',' )
//     {
//         if( is_array($Subject) === TRUE )
//             return $Surround.implode($Surround.$Delim.$Surround,
//                                     (strlen($Surround)===1?str_replace($Surround,"\\$Surround",$Subject):$Surround)).$Surround;
//         else
//             return NULL;
//     }

//     /**
//      * Create an array from a base array with certain elements masked out by key.
//      *
//      * @param array $Mask A numeric array of keys to mask.
//      * @param array $Haystack The base array.
//      * @retval array The new array.
//      */
//     public static function KeyMask( $Mask,$Haystack )
//     {
//         return array_diff_key($Haystack,array_fill_keys($Mask,TRUE));
//     }

//     /**
//      * Determine the absolute position of a key in an array.
//      *
//      * This method will consistently return the numeric position of an array's key, even if
//      * the array is associative or non-consecutive.
//      *
//      * The search is done using strict type checking.
//      *
//      * @param string $Needle The key to search for.
//      * @param array $Haystack The array to search.
//      * @retval int The position of $Needle.
//      * @retval NULL $Needle was not found.
//      */
//     public static function KeyPosition( $Needle,$Haystack )
//     {
//         return ($P = array_search($Needle,array_keys($Haystack),TRUE))===FALSE?NULL:$P;
//     }

// }


// /**
//  * Base class to encapsulate key/value pairs and their getter/setter logic
//  * for local memory-resident variables.
//  *
//  * A KeyValueSet (KVS) is a data structure and group of methods for iteration, getters,
//  * setters, etc.
//  */
// class KeyValueSet implements KVS,Directable
// {
//     /**
//      * @var array $KV
//      * Key/value pairs that comprise the KVS's data.
//      */
//     protected $KV = array();

//     /**
//      * @var int $KVLength
//      * The number of key/value pairs in this KVS.
//      *
//      * @note This and {@link $KVPosition} are used in a just-in-time fashion for iteraion.
//      * 		 They are not kept current during get/set/unset operations.
//      */
//     protected $KVLength = 0;

//     /**
//      * @var int $KVPosition
//      * The current position within KeyValueSet::$KVLength.
//      */
//     protected $KVPosition = 0;


//     /**
//      * Create a new KVS object.
//      *
//      * If ArrayRef isn't supplied, the KVS will reference it's own empty array.
//      *
//      * @param array|NULL &$ArrayRef Reference to an array which contains the KV's key/value pairs.
//      * @throws KV must reference an array, TYPE given.
//      */
//     public function __construct( &$ArrayRef = NULL )
//     {
//         if( is_array($ArrayRef) === TRUE )
//             $this->KV = &$ArrayRef;
//         else if( $ArrayRef !== NULL )
//             throw new Exception('KV must reference an array, '.gettype($ArrayRef).' given.');
//     }

//     /**
//      * Key/value getter using property overloading.
//      *
//      * @param integer|string $Key The element's key to get.
//      * @retval mixed|NULL The element's value or NULL if $Key isn't __isset().
//      */
//     public function __get( $Key )
//     {
//         return $this->__isset($Key)===TRUE?$this->KV[$Key]:NULL;
//     }

//     /**
//      * Key/value setter using property overloading.
//      *
//      * If $Key exists, it's value is silently overwritten.
//      *
//      * @param integer|string $Key The element's key to set.
//      * @param mixed $Value The element's value to set.
//      */
//     public function __set( $Key,$Value )
//     {
//         $this->KV[$Key] = $Value;
//     }

//     /**
//      * Determine if a key is set using property overloading.
//      *
//      * For performance, isset() is first used to check whether $Key
//      * is set (non-NULL).  This check is case-sensitive and loosely typed.
//      *
//      * If isset() returns FALSE, array_key_exists() is used
//      * to check if $Key is set, but is set to NULL.  This check is
//      * case-sensitive and loosley typed.
//      *
//      * @param integer|string $Key The element's key to check.
//      * @retval boolean TRUE if the $Key exists.
//      *
//      * @note Tempting PHP's loosely typed guesswork can lead to painfully subtle
//      * 	     bugs - use only non-numeric strings or actual integers as keys.
//      */
//     public function __isset( $Key )
//     {
//         return (isset($this->KV[$Key])===TRUE?TRUE:array_key_exists($Key,$this->KV));
//     }

//     /**
//      * Unset a key/value pair using property overloading.
//      *
//      * @param integer|string $Key The element's key to unset.
//      * @retval boolean TRUE if $Key was found.
//      */
//     public function __unset( $Key )
//     {
//         if( $this->__isset($Key) === TRUE )
//         {
//             $this->KV[$Key] = NULL;
//             unset($this->KV[$Key]);
//             return TRUE;
//         }
//         else
//         {
//             return FALSE;
//         }
//     }

//     /**
//      * @retval mixed
//      * @note Implement SPL's ArrayAccess interface.
//      */
//     public function offsetGet( $Key )
//     {
//         return $this->__get($Key);
//     }

//     /**
//      * @note Implement SPL's ArrayAccess interface.
//      */
//     public function offsetSet( $Key,$Value ): void
//     {
//         $this->__set($Key,$Value);
//     }

//     /**
//      * @retval boolean
//      * @note Implement SPL's ArrayAccess interface.
//      */
//     public function offsetExists( $Key ): bool
//     {
//         return $this->__isset($Key);
//     }

//     /**
//      * @note Implement SPL's ArrayAccess interface.
//      */
//     public function offsetUnset( $Key ): void
//     {
//         $this->__unset($Key);
//     }

//     /**
//      * Implement Directable interface.
//      *
//      * This allows key/value pairs to be set via manifest directives.
//      *
//      * @param string $Key The name of the key to set.
//      * @param mixed $Value The value to set.
//      */
//     public function ApplyDirective( $Key,$Value )
//     {
//         $this->KV[$Key] = $Value;
//     }

//     /**
//      * Set the KVS to reference another array.
//      *
//      * @param $ArrayRef A reference to an array.
//      */
//     public function Import( &$ArrayRef = NULL )
//     {
//         $this->KV = &$ArrayRef;
//     }

//     /**
//      * Return the referenced array.
//      * @retval array
//      */
//     public function Export()
//     {
//         return $this->KV;
//     }
// }

