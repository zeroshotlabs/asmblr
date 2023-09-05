<?php declare(strict_types=1);

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

