<?php
/**
 * @file Base.php Base classes.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * Directives allow keys/values to be pushed from the manifest into an object.
 */
interface Directable
{
    /**
     * Apply a directive.
     *
     * @param string $Key The key - or name - of the directive to set.
     * @param mixed $Value The value of the directive.
     */
    public function ApplyDirective( $Key,$Value );
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
 * Interface for classes which wish to manipulate a set key/value pairs.
 */
interface KVS extends \Iterator,\Countable,\ArrayAccess
{
    public function __get( $Key );

    public function __set( $Key,$Value );

    public function __isset( $Key );

    public function __unset( $Key );

    public function Export();
}

/**
 * Data Access Object
 * Flexible data object allowing array access mapped to properties by default.
 */
trait DAOt
{
    public function __get( $Key )
    {
        return isset($this->$Key)?$this->$Key:NULL;
    }

    public function offsetGet( $Key ): mixed
    {
        return isset($this->$Key)?$this->$Key:NULL;        
    }

    public function __set( $Key,$Value ): void
    {
        $this->$Key = $Value;
    }

    public function offsetSet( $Key,$Value ): void
    {
        $this->$Key = $Value;
    }

    public function __isset( $Key ): bool
    {
        return isset($this->$Key);
    }
    public function offsetExists( $Key ): bool
    {
        return $this->__isset($Key);        
    }
    public function __unset( $Key )
    {
        unset($this->$Key);
        return TRUE;
    }
    public function offsetUnset( $Key ): void
    {
        unset($this->$Key);
    }
}


/**
 * Default debugging methods for the Debuggable interface.
 *
 * @warning This won't appear in the docs because Doxygen doesn't support traits!
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
     * @retval boolean TRUE if debuggin is enabled.
     */
    public function IsDebug()
    {
        return !empty($_SERVER[$this->DebugToken]);
    }
}


/**
 * Interface for classes which wish to implement CREATE, READ, UPDATE,
 * DELETE and COUNT functionality on a data source.
 */
interface CRUDC
{
    public function CREATE( $Table,$Values );

    public function READ( $Table,$Constraint = NULL,$Columns = NULL,$OrderBy = NULL );

    public function UPDATE( $Table,$Values,$Constraint );

    public function DELETE( $Table,$Constraint );

    public function COUNT( $Table,$Constraint = NULL );
}


/**
 * Namespace-local exception.
 *
 * Thrown internally by asmblr.
 */
class Exception extends \Exception
{
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
     * @todo this needs update - $asmapp is old
     *
     * @note Chrome PHP doesn't announce itself so we're not supporting it anymore
     */
    public static function Log( $Msg,$Level = 'LOG',$Backtrace = NULL,$Context = NULL )
    {
        global $asmapp;

        $IsCLI = $asmapp->Request['IsCLI'];

        // if this isn't even set we likely have a problem very early on so just dump the message and hope
        if( !isset($asmapp->Config['LogPublic']) )
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


/**
 * Tools for including files and editing the include path.
 *
 * @note The include_path should be used sparingly, if at all.  Use absolute paths.
 * @note This class does not protect against security concerns or other mistakes.
 */
abstract class Inc
{
    /**
     * Add a path to the end of the include path.
     *
     * @param string $Path The path to add.
     * @throws Exception $Path is not a string.
     *
     * @note No normalization of $Path is done.
     */
    public static function Append( $Path )
    {
        if( is_string($Path) === FALSE )
            throw new Exception('Path is not a string.');

        set_include_path(get_include_path().DIRECTORY_SEPARATOR.$Path);
    }

    /**
     * Add a path to the beginning of the include path.
     *
     * @param string $Path The path to add.
     * @param boolean $OverCWD Set to TRUE to add the new path before the current working directory.
     * @throws Exception $Path is not a string.
     */
    public static function Prepend( $Path,$OverCWD = FALSE )
    {
        if( is_string($Path) === FALSE )
            throw new Exception('Path is not a string.');

        $C = get_include_path();

        if( $OverCWD === FALSE && $C[0] === '.' )
            set_include_path('.;'.$Path.substr($C,1));
        else
            set_include_path($Path.';'.$C);
    }

    /**
     * Load an extension that's bundled with asmblr under the ext/ directory.
     *
     * @c $ExtLoader is tried in the following way, relative to ext/:
     *   - literally as a filename
     *   - as a directory with a Load.inc
     * 
     * It is case-sensitive.
     *
     * @param string $ExtLoader The extension's loader filename or directory.
     *
     * @note This uses require() so pay attention.
     * @todo Improve path handling ASM_EXT_ROOT is hardwired currently.
     */
    public static function Ext( $ExtLoader )
    {
        if( is_file(ASM_EXT_ROOT.$ExtLoader) )
            require(ASM_EXT_ROOT.$ExtLoader);
        else if( is_dir(ASM_EXT_ROOT.$ExtLoader) )
            require(ASM_EXT_ROOT.$ExtLoader.'Load.inc');
    }
}


/**
 * Tools for common HTTP 1.1 headers and operations.
 *
 * This includes functions for common HTTP tasks, including redirecting, server errors, not found,
 * common content-types and file extensions, forcing the browser to download a file, and not caching content.
 *
 * @todo We may want the option to send useful cookies in this or another class
 *       (things like remember me, breadcrumbs/user path tracking).
 *
 * @see http://en.wikipedia.org/wiki/Internet_media_type
 * @see http://us2.php.net/manual/en/function.header.php
 */
abstract class HTTP
{
    /**
     * @var array $Types
     * Supported content types and their extensions.
     */
    public static $Types = array(
            'atom'=>'application/atom+xml',
            'rss'=>'application/rss+xml',

            'css'=>'text/css',
            'html'=>'text/html',
            'php'=>'text/x-php',
            'phtml'=>'application/x-httpd-php',

            'js'=>'text/javascript','javascript'=>'text/javascript','json'=>'application/json',
            'js4329'=>'application/javascript','javascript4329'=>'application/javascript',

            'xml'=>'text/xml',
            'app_xml'=>'application/xml',

            'text'=>'text/plain',
            'txt'=>'text/plain',
            'vcard'=>'text/vcard',
            'csv'=>'text/csv',

            'eot'=>'application/vnd.ms-fontobject','svg'=>'image/svg+xml','otf'=>'application/x-font-opentype',
            'ttf'=>'application/x-font-ttf','woff'=>'application/font-woff','woff2'=>'font/woff2',

            'gif'=>'image/gif','jpeg'=>'image/jpeg','jpg'=>'image/jpeg','png'=>'image/png',
            'ico'=>'image/x-icon','favicon'=>'image/x-icon',

            'zip'=>'application/zip','gzip'=>'application/x-gzip','gz'=>'application/x-gzip',

            'mp4'=>'video/mp4','webm'=>'video/webm',

            'pdf'=>'application/pdf',

            'excel'=>'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet',
            'xlsx'=>'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet',
            'xls'=>'application/vnd.ms-excel',

            'word'=>'application/vnd.openxmlformats-officedocument.wordprocessingml.document',
            'docx'=>'application/vnd.openxmlformats-officedocument.wordprocessingml.document',
            'doc'=>'application/msword',

            'powerpoint'=>'application/vnd.openxmlformats-officedocument.presentationml.presentation',
            'pptx'=>'application/vnd.openxmlformats-officedocument.presentationml.presentation',
            'ppt'=>'application/vnd.ms-powerpoint',

            'binary'=>'application/octet-stream',

            'form'=>'application/x-www-form-urlencoded');

    /**
     * Send a 200 OK header.
     */
    public static function _200()
    {
        header('HTTP/1.1 200 OK');
    }

    /**
     * Send a 204 No Content header (typically for OPTIONS responses).
     *
     * @param boolean $Exit FALSE to not kill execution after sending the header.
     */
    public static function _204( $Exit = TRUE )
    {
        header('HTTP/1.1 204 No Content');
        if( $Exit === TRUE )
            exit;
    }

    /**
     * Send a 301 Moved Permanently header.
     */
    public static function _301()
    {
        header('HTTP/1.1 301 Moved Permanently');
    }

    /**
     * Send a 302 Found header.
     */
    public static function _302()
    {
        header('HTTP/1.1 302 Found');
    }

    /**
     * Send a 400 Bad Request header.
     *
     * @param boolean $Exit FALSE to not kill execution after sending the header.
     */
    public static function _400( $Exit = TRUE )
    {
        header('HTTP/1.1 400 Bad Request');
        if( $Exit === TRUE )
            exit;
    }

    /**
     * Send a 401 Unauthorized header.
     *
     * @param boolean $Exit FALSE to not kill execution after sending the header.
     */
    public static function _401( $Exit = TRUE )
    {
        header('HTTP/1.1 401 Unauthorized');
        if( $Exit === TRUE )
            exit;
    }

    /**
     * Send a 403 Forbidden header.
     *
     * @param boolean $Exit FALSE to not kill execution after sending the header.
     */
    public static function _403( $Exit = TRUE )
    {
        header('HTTP/1.1 403 Forbidden');
        if( $Exit === TRUE )
            exit;
    }

    /**
     * Send a 404 Not Found header.
     */
    public static function _404()
    {
        header('HTTP/1.1 404 Not Found');
    }

    /**
     * Send a 500 Internal Server Error header.
     *
     * @param boolean $Exit FALSE to not kill execution after sending the header.
     */
    public static function _500( $Exit = TRUE )
    {
        header('HTTP/1.1 500 Internal Server Error');
        if( $Exit === TRUE )
            exit;
    }

    /**
     * Send a 501 Not Implemented header.
     *
     * @param boolean $Exit FALSE to not kill execution after sending the header.
     */
    public static function _501( $Exit = TRUE )
    {
        header('HTTP/1.1 501 Not Implemented');
        if( $Exit === TRUE )
            exit;
    }

    /**
     * Send a 503 Service Unavailable header.
     *
     * @param boolean $Exit FALSE to not kill execution after sending the header.
     */
    public static function _503( $Exit = TRUE )
    {
        header('HTTP/1.1 503 Service Unavailable');
        if( $Exit === TRUE )
            exit;
    }

    /**
     * Send a Location header for redirecting.
     *
     * @param string $URL The URL to redirect to.
     * @param URL $URL The URL Struct to redirect to.
     * @param boolean $Perm FALSE to not send a 301 header first.
     * @param boolean $Exit FALSE to not kill execution after sending the header.
     */
    public static function Location( $URL,$Perm = TRUE,$Exit = TRUE )
    {
        if( $Perm === TRUE )
            static::_301();

        header('Location: '.(is_array($URL)?URL::ToString($URL):$URL));

        if( $Exit === TRUE )
            exit;
    }

    /**
     * Send no-caching headers.
     */
    public static function NoCache()
    {
        header('Cache-Control: no-cache, must-revalidate');
        header("Expires: Sat, 1 Jan 2000 00:00:00 GMT");
    }

    /**
     * Send absolute caching headers.
     *
     * @param int $Seconds The duration, in seconds, to cache for.
     *
     * @note This is "absolute" caching - the browser won't even request the resource for the duration specified.
     *
     * @see HTTP::LastModified for conditional caching.
     * @see https://developers.google.com/speed/articles/caching
     */
    public static function Cache( $Seconds )
    {
        header('Expires: '.date('r',strtotime("+ $Seconds seconds")));
        header("Cache-Control: max-age={$Seconds}");
        header('Pragma: public');
    }

    /**
     * Send a last modified header.
     *
     * @param string $DateTime A strtotime() compatible string.
     * @param int $DateTime A Unix timestamp.
     * @param NULL $DateTime Default of NULL for current date/time.
     * @retval boolean TRUE if a valid date was determined and the header sent, FALSE if no header is sent.
     *
     * @note This is conditional caching - browser checks if resource has been modified more
     *       recently than the DateTime specified here.
     *
     * @see Cache() for absolute caching.
     */
    public static function LastModified( $DateTime = NULL )
    {
        if( $DateTime === NULL )
            $DateTime = date('r');
        else if( is_int($DateTime) === TRUE )
            $DateTime = date('r',$DateTime);
        else if( is_string($DateTime) === TRUE )
            $DateTime = @date('r',strtotime($DateTime));
        else
            $DateTime = FALSE;

        if( empty($DateTime) )
        {
            return FALSE;
        }
        else
        {
            header("Last-Modified: $DateTime");
            return TRUE;
        }
    }

    public static function RetryAfter( $Seconds = 30 )
    {
        header("Retry-After: $Seconds");
        return TRUE;
    }

    /**
     * Send a Content-Type header.
     *
     * @param string $Type Case insensitive name (extension) of a common content-type, or if it contains a forward slash it's sent as is.
     * @param string $Charset Specify the charset attribute.
     *
     * @note Some content-types can be referenced by multiple names, like a common
     *       name and file extension.  If a type isn't recognized, application/octet-stream is sent.
     * @note $Charset is passed untouched.
     *
     * @todo We should have methods for content-disposition (attachment, download, etc).
     *
     * @see $Types property for available names.
     */
    public static function ContentType( $Type,$Charset = '' )
    {
        if( $Type === Null )
            llog($_SERVER['REQUEST_URI']);
        else
            header('Content-Type: '.static::ResolveContentType($Type).(empty($Charset)?'':"; charset={$Charset}"));
    }

    /**
     * Send the headers to force a file download/save-as.
     *
     * This doesn't actually send the file.
     *
     * @param string $Filename The filename to save-as.
     * @param string $Type The content-type name.
     * @param string $Length The content-length, if known.
     *
     * @note Content length typically shouldn't be set because it causes IIS to reset the connection.
     *
     * @todo No crazy encoding of filename is done - suggestions welcome.
     * @todo No content-transfer-encoding is supported currently.
     */
    public static function SaveAs( $Filename,$Type = NULL,$Length = NULL )
    {
        static::ContentType($Type);

        header("Content-Disposition: attachment; filename=\"{$Filename}\"");
        header('Cache-Control: must-revalidate');
        header('Pragma: ');

        if( $Length !== NULL )
            header("Content-Length: $Length}");
    }

    /**
     * Send default headers to support CORS, including handling an OPTIONS request.
     *
     * This allows from any origin, GET/POST/OPTIONS methods and most headers.
     *
     * @todo This currently supports only generic default behavior.  Needs parameters to fine tune/restrict.
     *
     * @see http://enable-cors.org/server_nginx.html for nginx handling.
     * @see https://developer.mozilla.org/en-US/docs/Web/HTTP/Access_control_CORS
     */
    public static function CORS()
    {
        // support OPTIONS pre-flight with any origin and exit
        if( $_SERVER['REQUEST_METHOD'] === 'OPTIONS' )
        {
            if( !empty($_SERVER['HTTP_ORIGIN']) )
                header("Access-Control-Allow-Origin: {$_SERVER['HTTP_ORIGIN']}");

            header('Access-Control-Allow-Credentials: true');
            header('Access-Control-Max-Age: 25');    // cache for 25 seconds
            header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
            header('Access-Control-Allow-Headers: Authorization,Content-Type,Accept,Origin,User-Agent,DNT,Cache-Control,X-Mx-ReqToken,Keep-Alive,X-Requested-With,If-Modified-Since');

            header('Content-Length: 0');
            HTTP::_204();
        }
        // allow from any origin
        else if (isset($_SERVER['HTTP_ORIGIN']))
        {
            header('Access-Control-Allow-Credentials: true');
            header('Access-Control-Max-Age: 25');    // cache for 25 seconds
            header("Access-Control-Allow-Origin: {$_SERVER['HTTP_ORIGIN']}");
        }
    }

    /**
     * Resolve a content type name to it's proper MIME value, or binary (application/octet-stream)
     * if it can't be resolved.
     *
     * @param string $Type Case insensitive name (extension) of a common content-type, or if it contains a forward slash it's sent as is.
     * @param bool $Strict Return NULL instead of binary if the type can't be determined.
     * @retval NULL The content type couldn't be resolved and $Strict was set.
     * @retval string The fully qualified MIME content type.
     */
    public static function ResolveContentType( $Type,$Strict = FALSE )
    {
        if( strpos($Type,'/') !== FALSE )
        {
            return strtolower($Type);
        }
        else
        {
            $Type = strtolower($Type);
            return (isset(static::$Types[$Type])===TRUE?static::$Types[$Type]:($Strict===TRUE?NULL:static::$Types['binary']));
        }
    }

    /**
     * Return the short-names of the known content types.
     *
     * @param bool $Full TRUE to return full content types.
     * @retval array An array of known content types.
     */
    public static function GetContentTypes( $Full = FALSE )
    {
        if( $Full === TRUE )
            return static::$Types;
        else
            return array_keys(static::$Types);
    }

    /**
     * Try to determine the content type from a filename's extension.
     *
     * @param string $Filename The filename to resolve.
     * @retval string The content type.
     * @retval NULL The content type could not be determined.
     */
    public static function Filename2ContentType( $Filename )
    {
        $Ext = pathinfo($Filename);
        $Ext = strtolower(Struct::Get('extension',$Ext));

        return isset(static::$Types[$Ext])===TRUE?static::$Types[$Ext]:NULL;
    }
}


/**
 * A Struct manages arrays that have an overall consistent structure.
 *
 * This is a base class that's meant to be extended for application specific data structures.
 *
 * Structs support both associative and integer indexed arrays, or even a mixed
 * array.  However, mixing the two in a single array is not recommended, and can cause
 * unpredictable results.
 *
 * For the methods with a RefPoint parameter, these rules are used:
 *
 *  - When RefPoint is an integer, working with an integer array is assumed and keys are not maintained.
 *  - When RefPoint is a string, working with an associative array is assumed and keys are maintained.
 *
 * All methods that insert a new Element will attempt to insert it as a whole - only one
 * new element will be created.
 *
 * By convention, the general structure of a Struct managed array is defined in the
 * $Skel property.  A new Struct array is created by an Init() method and returned.
 *
 * A general way to think of Structs is as the minimal amount of elements and their
 * expected values/types that are needed for a purpose.  The array may contain more
 * elements.
 *
 * @see Is for methods that operate on elements of generic arrays.
 */
abstract class Struct
{
    /**
     * Default structure for a Struct.
     */
    protected static $Skel = array();


    /**
     * Determine whether the array has a known structure.
     *
     * This compares the keys of $Arr to that of Struct::$Skel.
     *
     * If a more complete comparision, including type checking, is needed,
     * see Is::Equal().
     *
     * @param array $Arr The array to check.
     * @retval boolean TRUE if the keys are known.
     */
    public static function IsA( $Arr )
    {
        return (array_keys($Arr) === array_keys(static::$Skel));
    }

    /**
     * Append a new element to the array.
     *
     * @param mixed $Element Element to append.
     * @param array $Subject The array to append to.
     * @throws Exception Element with key already exists.
     * @throws Exception Invalid Element.
     * @retval int The position, counting from 0, at which the element was appended.
     */
    public static function Append( $Element,&$Subject )
    {
        if( is_array($Element) === FALSE )
        {
            $Subject[] = $Element;
            return count($Subject)-1;
        }
        else if( count($Element) === 1 )
        {
            if( isset($Subject[key($Element)]) === FALSE || is_int(key($Element)) === TRUE )
            {
                $Subject = array_merge($Subject,$Element);
                return count($Subject)-1;
            }
            else
                throw new Exception('Element with key '.key($Element).' already exists.');
        }
        else
            throw new Exception('Invalid Element.');
    }

    /**
     * Prepend a new element to the array.
     *
     * @param mixed $Element Element to prepend.
     * @param array $Subject The array to prepend to.
     * @throws Exception Element with key already exists.
     * @throws Exception Invalid Element.
     * @retval int 0
     */
    public static function Prepend( $Element,&$Subject )
    {
        if( is_array($Element) === FALSE )
        {
            array_splice($Subject,0,0,$Element);
            return 0;
        }
        else if( count($Element) === 1 )
        {
            if( isset($Subject[key($Element)]) === FALSE || is_int(key($Element)) === TRUE )
            {
                //array_splice($Subject,0,0,$Element);
                $Subject = array_merge($Element,$Subject);
                return 0;
            }
            else
                throw new Exception('Element with key '.key($Element).' already exists.');
        }
        else
            throw new Exception('Invalid Element.');
    }

    /**
     * Insert a new element before another element.
     *
     * Element is inserted as a whole, regardless of it's keys, if any.
     *
     * @param mixed $Element Element to insert.
     * @param int|string $RefPoint Reference point of insertion as either an associative or numeric key.
     * @param array &$Subject The array to insert into.
     * @throws Exception Element with key already exists.
     * @throws Exception Invalid RefPoint or Element.
     * @retval int The position, counting from 0, at which insertion occurred.
     * @retval NULL RefPoint was not found and nothing was inserted.
     *
     * @note This probably isn't bullet proof.
     */
    public static function InsertBefore( $Element,$RefPoint,&$Subject )
    {
        if( is_int($RefPoint) === TRUE )
        {
            if( isset($Subject[$RefPoint]) === TRUE )
            {
                array_splice($Subject,$RefPoint,0,is_array($Element)===TRUE?array($Element):$Element);
                return $RefPoint;
            }
            else
                return NULL;
        }
        else if( is_string($RefPoint) === TRUE && is_array($Element) === TRUE )
            // && count($Element) === 1
        {
            if( isset($Subject[key($Element)]) === FALSE )
            {
                if( ($RefPoint = static::KeyPosition($RefPoint,$Subject)) === NULL )
                    return NULL;
                $Subject = array_merge(array_splice($Subject,0,$RefPoint),$Element,$Subject);
                return $RefPoint;
            }
            else
                throw new Exception('Element with key '.key($Element).' already exists.');
        }
        else
            throw new Exception('Invalid RefPoint or Element.');
    }

    /**
     * Insert a new element after another element.
     *
     * Element is inserted as a whole, regardless of it's keys, if any.  If it's an array, it can
     * have only a single key/value pair (unlike InsertBefore).
     *
     * @param mixed $Element Element to insert.
     * @param int|string $RefPoint Reference point of insertion as either an associative or numeric key.
     * @param array &$Subject The array to insert into.
     * @throws Exception Element with key already exists.
     * @throws Exception Invalid RefPoint or Element.
     * @retval int The position, counting from 0, at which insertion occurred.
     * @retval NULL RefPoint was not found and nothing was inserted.
     *
     * @note This probably isn't bullet proof.
     */
    public static function InsertAfter( $Element,$RefPoint,&$Subject )
    {
        if( is_int($RefPoint) === TRUE )
        {
            if( isset($Subject[$RefPoint]) === TRUE )
            {
                array_splice($Subject,++$RefPoint,0,is_array($Element)===TRUE?array($Element):$Element);
                return $RefPoint;
            }
            else
                return NULL;
        }
        else if( is_string($RefPoint) === TRUE && is_array($Element) === TRUE && count($Element) === 1 )
        {
            if( isset($Subject[key($Element)]) === FALSE )
            {
                if( ($RefPoint = static::KeyPosition($RefPoint,$Subject)) === NULL )
                    return NULL;
                $Subject = array_merge(array_splice($Subject,0,++$RefPoint),$Element,$Subject);
                return $RefPoint;
            }
            else
                throw new Exception('Element with key '.key($Element).' already exists.');
        }
        else
            throw new Exception('Invalid RefPoint or Element.');
    }

    /**
     * Remove an element from an array.
     *
     * If $Needle is an integer, the remaining keys will be reindexed.  In an associative,
     * $Needle indicates the position of the element to remove, not it's actual key.
     *
     * @param int|string $Needle Key of the element to remove.
     * @param array &$Haystack The array to remove the element from.
     * @retval boolean TRUE if $Needle was found and it's element removed.
     */
    public static function Del( $Needle,&$Haystack )
    {
        if( isset($Haystack[$Needle]) === TRUE )
        {
            if( is_int($Needle) === TRUE )
                array_splice($Haystack,$Needle,1);
            else
                unset($Haystack[$Needle]);

            return TRUE;
        }
        else
            return FALSE;
    }

    /**
     * Read or check the value of an element.
     *
     * If $Check is provided, strict type checking is used.
     *
     * @param int|string $Needle Key of the element to read.
     * @param array $Haystack The array to read the element from.
     * @param mixed $Check Optional value to check the element's value against.
     * @retval mixed The element's value if no $Check was provided.
     * @retval NULL $Needle was not found and no $Check was provided.
     * @retval boolean TRUE if $Check was provided and the values matched.
     */
    public static function Get( $Needle,$Haystack,$Check = NULL )
    {
        if( isset($Haystack[$Needle]) === TRUE )
            return ($Check===NULL)?$Haystack[$Needle]:($Haystack[$Needle]===$Check);
        else
            return NULL;
    }

    /**
     * Convert the values of an array into a string.
     *
     * @param array $Subject The array to convert.
     * @param string $Surround A string to surround each value with.
     * @param string $Delim A string to seperate each value.
     * @retval string The array values as a string.
     * @retval NULL Subject was not an array.
     *
     * @note If $Surround is a single character and appears in a value, it is escaped with a backslash.
     */
    public static function Dissolve( $Subject,$Surround = '\'',$Delim = ',' )
    {
        if( is_array($Subject) === TRUE )
            return $Surround.implode($Surround.$Delim.$Surround,
                                    (strlen($Surround)===1?str_replace($Surround,"\\$Surround",$Subject):$Surround)).$Surround;
        else
            return NULL;
    }

    /**
     * Get a column of values from a Is::Columnar() array.
     *
     * @param string $Needle The name of the column to get.
     * @param array $Haystack A columnar array.
     * @retval array The column's values.
     * @retval NULL The array isn't columnar.
     *
     * @note If some "rows" don't have the column specified, the NULL value will be filled in.
     */
    public static function GetColumn( $Needle,$Haystack )
    {
        if( Is::Columnar($Haystack) === TRUE )
        {
            $Column = array();
            foreach( $Haystack as $V )
                $Column[] = (isset($V[$Needle])===TRUE?$V[$Needle]:NULL);
            return $Column;
        }
        else
            return NULL;
    }

    /**
     * Create an array from a base array with certain elements masked out by key.
     *
     * @param array $Mask A numeric array of keys to mask.
     * @param array $Haystack The base array.
     * @retval array The new array.
     */
    public static function KeyMask( $Mask,$Haystack )
    {
        return array_diff_key($Haystack,array_fill_keys($Mask,TRUE));
    }

    /**
     * Determine the absolute position of a key in an array.
     *
     * This method will consistently return the numeric position of an array's key, even if
     * the array is associative or non-consecutive.
     *
     * The search is done using strict type checking.
     *
     * @param string $Needle The key to search for.
     * @param array $Haystack The array to search.
     * @retval int The position of $Needle.
     * @retval NULL $Needle was not found.
     */
    public static function KeyPosition( $Needle,$Haystack )
    {
        return ($P = array_search($Needle,array_keys($Haystack),TRUE))===FALSE?NULL:$P;
    }

    /**
     * Convert a string, or recursively convert an array, to UTF-8.
     *
     * @param array,string $A A reference to the array to convert.
     *                        A reference to the string to convert.
     *
     * @todo Full testing when fed strangely encoded strings.
     * @note Changes original value.
     */
    public static function ToUTF8( &$A ): void
    {
        if( is_array($A) === TRUE )
        {
            foreach( $A as &$V )
            {
                // Avoid encoding already encoded UTF-8 - TRUE is required to make a strict test.
                if( is_string($V) === TRUE && mb_detect_encoding($V,'UTF-8, ISO-8859-1',TRUE) !== 'UTF-8')
                    $V = mb_convert_encoding($V,'UTF-8');
                else if( is_array($V) === TRUE )
                    static::ToUTF8($V);
            }
        }
        else if( is_string($A) === TRUE )
        {
            if( mb_detect_encoding($A,'UTF-8, ISO-8859-1',TRUE) !== 'UTF-8')
                $A = mb_convert_encoding($A,'UTF-8');
        }
    }
}


/**
 * Helper methods for checking variables and elements of any array.
 */
abstract class Is
{
    /**
     * Determine whether two arrays have the same structure.
     *
     * Two arrays have the same structure when:
     *  - elements occur in the same order and have the same keys.
     *  - the values of each element are of the same type (using What()).
     *
     * The values of each element are not compared - only their types are.
     *
     * @param array $Subject1 The first array to compare.
     * @param array $Subject2 The second array to compare.
     * @param boolean $Recurse \c TRUE to recursively compare arrays.
     * @param array $Ignore An array of elements to ignore.
     * @throws Exception \c Subject1 is not an array.
     * @throws Exception \c Subject2 is not an array.
     * @retval boolean \c TRUE if the arrays have the same structure.
     *
     * @note All comparisions are type strict (===).
     * @todo This is probably broken.
     * @todo Skip the value check (i.e., always TRUE) if the type is NULL.
     */
    public static function Equal( $Subject1,$Subject2,$Recurse = FALSE,$Ignore = array() )
    {
        if( is_array($Subject1) === FALSE )
            throw new Exception('Subject1 is not an array.');
        else if( is_array($Subject2) === FALSE )
            throw new Exception('Subject2 is not an array.');

        $S1 = Struct::KeyMask($Ignore,$Subject1);
        $S2 = Struct::KeyMask($Ignore,$Subject2);

        if( array_keys($S1) === array_keys($S2) )
        {
            foreach( $S1 as $K => $V )
            {
                if( ($Recurse === TRUE) && (is_array($S1[$K]) === TRUE && is_array($S2[$K]) === TRUE) )
                {
                    if( self::Equal($S1[$K],$S2[$K],$Ignore) === FALSE )
                        return FALSE;
                }
                else if( (static::What($K,$S1) !== static::What($K,$S2)) )
                {
                    return FALSE;
                }
            }

            return TRUE;
        }
        else
            return FALSE;
    }

    /**
     * Determine the type of a variable or a value in an array.
     *
     * Uses PHP's \c is_* functions to make the determination and returns
     * the name of the function after the \c is_* part.  Thus, the returned
     * string is one of:
     * 	@li @c string
     *  @li @c int
     *  @li @c bool
     *  @li @c array
     *  @li @c object
     *  @li @c null
     *  @li @c float
     *
     * @param mixed $Needle The value to check or the key of an array to check.
     * @param array $Haystack The array to search, or NULL.
     * @retval string The name of the type.
     * @retval NULL The value's type isn't known.
     * @retval FALSE The $Needle wasn't found in $Haystack.
     *
     * @todo Possibly handle \c is_numeric() vs \c is_int().
     * @todo Possibly handle \c is_scalar().
     */
    public static function What( $Needle,$Haystack = NULL )
    {
        if( is_array($Haystack) === TRUE )
        {
            if( array_key_exists($Needle,$Haystack) === TRUE )
                $Needle = $Haystack[$Needle];
            else
                return FALSE;
        }

        if( is_string($Needle) === TRUE )
            return 'string';
        else if( is_int($Needle) === TRUE )
            return 'int';
        else if( is_bool($Needle) === TRUE )
            return 'bool';
        else if( is_array($Needle) === TRUE )
            return 'array';
        else if( is_object($Needle) === TRUE )
            return 'object';
        else if( is_null($Needle) === TRUE )
            return 'null';
        else if( is_float($Needle) === TRUE )
            return 'float';
        else
            return NULL;
    }

    /**
     * Return TRUE if $Needle exists in $Haystack and is a string.
     */
    public static function String( $Needle,$Haystack )
    {
        return (isset($Haystack[$Needle]) === TRUE && is_string($Haystack[$Needle]) === TRUE);
    }

    /**
     * Return TRUE if $Needle exists in $Haystack and is an integer.
     *
     * @note This method returns \c TRUE for both strings and integer types.
     * @note IsFloat/IsNumeric are not currently implemented.
     */
    public static function Int( $Needle,$Haystack )
    {
        return (isset($Haystack[$Needle]) === TRUE && (ctype_digit($Haystack[$Needle]) === TRUE || is_int($Haystack[$Needle]) === TRUE) );
    }

    /**
     * Return TRUE if $Needle exists in $Haystack and is a boolean.
     */
    public static function Bool( $Needle,$Haystack )
    {
        return (isset($Haystack[$Needle]) === TRUE && is_bool($Haystack[$Needle]) === TRUE);
    }

    /**
     * Return TRUE if $Needle exists in $Haystack and is boolean TRUE.
     */
    public static function TRUE( $Needle,$Haystack )
    {
        return (isset($Haystack[$Needle]) === TRUE && $Haystack[$Needle] === TRUE);
    }

    /**
     * Return TRUE if $Needle exists in $Haystack and is boolean FALSE.
     */
    public static function FALSE( $Needle,$Haystack )
    {
        return (isset($Haystack[$Needle]) === TRUE && $Haystack[$Needle] === FALSE);
    }

    /**
     * Return TRUE if $Needle exists in $Haystack and is an array.
     */
    public static function Arr( $Needle,$Haystack )
    {
        return (isset($Haystack[$Needle]) === TRUE && is_array($Haystack[$Needle]) === TRUE);
    }

    /**
     * Return TRUE if $Needle exists in $Haystack and is an object.
     */
    public static function Object( $Needle,$Haystack )
    {
        return (isset($Haystack[$Needle]) === TRUE && is_object($Haystack[$Needle]) === TRUE);
    }

    /**
     * Return TRUE if $Needle exists in $Haystack and is a NULL value.
     */
    public static function NULL( $Needle,$Haystack )
    {
        return (is_array($Haystack) === TRUE && array_key_exists($Needle,$Haystack) === TRUE && $Haystack[$Needle] === NULL);
    }

    /**
     * Determine if all keys of an array are integers.
     *
     * @param array $Haystack The array to check.
     * @retval boolean TRUE if the array is numericly indexed.
     * @retval NULL Haystack is not an array.
     *
     * @note This does not check whether the keys are in sequential order.
     */
    public static function Numeric( $Haystack )
    {
        if( is_array($Haystack) === TRUE )
        {
            foreach( $Haystack as $K => $V )
                if( is_int($K) === FALSE )
                    return FALSE;

            return TRUE;
        }
        else
            return NULL;
    }

    /**
     * Determine if all keys of an array are strings.
     *
     * @param array $Haystack The array to check.
     * @retval boolean TRUE if the array is associativly indexed.
     * @retval NULL Haystack is not an array.
     */
    public static function Assoc( $Haystack )
    {
        if( is_array($Haystack) === TRUE )
        {
            foreach( $Haystack as $K => $V )
                if( is_string($K) === FALSE )
                    return FALSE;

            return TRUE;
        }
        else
            return NULL;
    }

    /**
     * Determine if the array is an numerically indexed array of other arrays (columns).
     *
     * @param array $Haystack The array to check.
     * @retval boolean TRUE if the array is columnar.
     * @retval NULL Haystack is not an array.
     *
     * @todo This needs to be tested.
     */
    public static function Columnar( $Haystack )
    {
        if( is_array($Haystack) === TRUE )
        {
            foreach( $Haystack as $Key => $Column )
            {
                if( is_int($Key) === FALSE || is_array($Column) === FALSE )
                    return FALSE;
            }

            return TRUE;
        }
        else
            return NULL;
    }

    /**
     * Check if certain keys exist in an array.
     *
     * @param scalar $Needles A single key to check for.
     * @param array $Needles An array of keys (as values) to check for.
     * @param array $Haystack The array to check.
     * @retval boolean TRUE if all keys are present.
     *
     * @note array_key_exists() is used, thus the check is case-sensitive, loosely typed, and will
     *       return TRUE if the element exists but is set to NULL.
     */
    public static function Keys( $Needles,$Haystack )
    {
        foreach( ((array) $Needles) as $N )
        {
            if( array_key_exists($N,$Haystack) === FALSE )
                return FALSE;
        }

        return TRUE;
    }

    /**
     * Check if certain values exist in an array.
     *
     * @param scalar $Needles A single value to check for.
     * @param array $Needles An array of values to check for.
     * @param array $Haystack The array to check.
     * @retval boolean TRUE if all values are present.
     *
     * @note Type-strict checking is performed using in_array().
     */
    public static function Values( $Needles,$Haystack )
    {
        foreach( ((array) $Needles) as $N )
        {
            if( in_array($N,$Haystack,TRUE) === FALSE )
                return FALSE;
        }

        return TRUE;
    }
}


/**
 * Tools for working with a path.
 *
 * By default, a Path uses the forward slash @c / as a separator.
 *
 * The structure of a Path Struct is:
 *
 *  @li @c Separator: the path's separator.
 *  @li @c IsAbs: TRUE if the path has a leading separator.
 *  @li @c IsDir: TRUE if the path has a trailing separator.
 *  @li @c IsRoot: if the path is only the separator (IsDir and IsAbs will also be TRUE).
 *  @li @c Segments: numeric array of the pieces between the separators.
 *
 * A path segment is what's contained between two separators, or a separator
 * and the end of the string.
 *
 * @note This is also used for the path part of a URL.
 * @note No automatic encoding/decoding/escaping is done.
 * @note This is platform agnostic - it doesn't know if it's running under Windows or Unix.
 * @note This is not a security mechanism - it does not watch for insecure paths, like @c /../../
 */
abstract class Path extends Struct
{
    protected static $Skel = array('Separator'=>'/','IsAbs'=>TRUE,'IsDir'=>TRUE,
    							   'IsRoot'=>TRUE,'Segments'=>array());

    /**
     * Create a Path from a string.
     *
     * A back-slash separator is automatically detected if there is one.
     *
     * @param string $PathStr The path string to parse, an empty string, or NULL.
     * @param string $Separator Specify a single character as a separator to use.
     * @retval array The Path Struct.
     *
     * @note An empty or NULL $PathStr, or one that is simply multiple separators,
     * 		 will be considered a root path.  A non-string is cast.
     */
    public static function Init( $PathStr,$Separator = NULL )
    {
        $P = static::$Skel;

        if( empty($Separator) )
        {
            if( strpos($PathStr,'\\') !== FALSE )
                $P['Separator'] = '\\';
        }
        else
            $P['Separator'] = $Separator;

        $PathStr = trim($PathStr);

        // a root path
        if( empty($PathStr) || $PathStr === $P['Separator'] || trim($PathStr,$P['Separator']) === '' )
        {
            $P['Segments'][0] = $P['Separator'];
        }
        else
        {
            $P['IsRoot'] = FALSE;
            $P['IsAbs'] = $PathStr[0]===$P['Separator']?TRUE:FALSE;
            $P['IsDir'] = substr($PathStr,-1,1)===$P['Separator']?TRUE:FALSE;
            $P['Segments'] = preg_split("(\\{$P['Separator']}+)",$PathStr,-1,PREG_SPLIT_NO_EMPTY);
        }

        return $P;
    }

    /**
     * Create a string from the Path array.
     *
     * If $EncodeType is provided, the returned path string is encoded using either URL or shell
     * semantics.
     *
     * @param Path $Path A Path Struct.
     * @param string $EncodeType One of @c url, @c shell.
     * @param NULL $EncodeType Do not perform any encoding.
     * @throws Exception Unknown EncodeType.
     * @retval string The path string.
     *
     * @todo Make this faster - it's used a lot.
     */
    public static function ToString( $Path,$EncodeType = NULL )
    {
        if( empty($Path) )
            return '';

        if( $Path['IsRoot'] === TRUE )
        {
            return $Path['Separator'];
        }
        else
        {
            $Segs = '';
            if( $EncodeType === NULL )
            {
                $Segs = implode($Path['Separator'],$Path['Segments']);
            }
            else
            {
                $EncodeType = strtolower($EncodeType);

                if( $EncodeType === 'url' )
                {
                    foreach( $Path['Segments'] as $K => $V )
                        $Segs .= ($K>0?$Path['Separator']:'').rawurlencode($V);
                }
                else if( $EncodeType === 'shell' )
                {
                    foreach( $Path['Segments'] as $K => $V )
                        $Segs .= ($K>0?$Path['Separator']:'').escapeshellcmd($V);
                }
                else
                    throw new Exception("Unknown EncodeType '{$EncodeType}'.");
            }

            return ($Path['IsAbs']===TRUE?$Path['Separator']:'').$Segs.($Path['IsDir']===TRUE?$Path['Separator']:'');
        }
    }

    /**
     * Create a URL encoded string from the Path.
     *
     * @param Path $Path A Path Struct.
     * @retval string The path string.
     */
    public static function ToURLString( $Path )
    {
        return static::ToString($Path,'url');
    }

    /**
     * Create a shell encoded string from the Path.
     *
     * @param Path $Path A Path Struct.
     * @retval string The path string.
     */
    public static function ToShellString( $Path )
    {
        return static::ToString($Path,'shell');
    }

    /**
     * Make all Segments of a Path Struct lowercase.
     *
     * @param Path $Path A reference to a Path Struct.
     */
    public static function Lower( &$Path )
    {
        foreach( $Path['Segments'] as &$V )
            $V = strtolower($V);
    }

    /**
     * Merge $Src segments into $Dest segments.
     *
     * If $Src has the same segment at the same position as $Dest, it is
     * skipped.  Otherwise, $Src segments are appended to $Dest.
     *
     * @param Path $Src The Path to merge in.
     * @param Path $Dest A reference to the base Path to merge into.
     *
     * @note All comparisions are type strict (===).
     * @note This is a no-op if $Src IsRoot is TRUE.
     * @note $Dest will have same IsDir as $Src.
     * @note $Dest will become IsRoot FALSE if the merge occurs.
     */
    public static function Merge( $Src,&$Dest )
    {
        if( $Src['IsRoot'] === TRUE )
            return;

        if( $Dest['IsRoot'] === TRUE )
            $Dest['Segments'] = array();

        $Dest['IsDir'] = $Src['IsDir'];
        $Dest['IsRoot'] = FALSE;

        foreach( $Src['Segments'] as $K => $V )
        {
            if( isset($Dest['Segments'][$K]) === TRUE && $Dest['Segments'][$K] === $V )
                continue;
            else
                $Dest['Segments'][] = $V;
        }
    }

    /**
     * Remove matching segments that exist in $Mask from $Base.
     *
     * If $Mask contains the same segments in the same positions as $Base,
     * remove those matching segments from $Base.
     *
     * @param Path $Mask The Path that masks segments.
     * @param Path $Base The Path that will have segments removed.
     * @retval void
     *
     * @note This is implemented using array_diff() and array_values().
     * @note This is a no-op if either $Mask or $Base is IsRoot TRUE.
     * @note This may cause $Base to become IsRoot and IsDir TRUE.
     */
    public static function Mask( $Mask,&$Base )
    {
        if( $Mask['IsRoot'] === TRUE || $Base['IsRoot'] === TRUE )
            return;

        $Base['Segments'] = array_values(array_diff_assoc($Base['Segments'],$Mask['Segments']));
        if( empty($Base['Segments']) )
        {
            $Base['Segments'][0] = $Base['Separator'];
            $Base['IsDir'] = $Base['IsRoot'] = TRUE;
        }
    }

    /**
     * Determine if a string appears to be an absolute path.
     *
     * A string is considered an absolute path under the following conditions:
     *  - The first character is a forward slash or a backslash.
     *  - The second character is a colon (for Windows paths).
     *
     * @param string $Path The string to check.
     * @throws Exception Path is not a string.
     * @retval boolean TRUE if the string is an absolute path.
     */
    public static function IsAbs( $Path )
    {
        if( is_string($Path) === FALSE )
            throw new Exception("Path is not a string.");

        return ((strpos($Path,'/')===0) || (strpos($Path,'\\')===0) || (strpos($Path,':')===1));
    }

    /**
     * Return TRUE if $Child is a child path of $Parent.
     *
     * The follow semantics also apply:
     *  - If both $Child and $Parent IsRoot is TRUE, this returns FALSE.
     *  - If $Child IsRoot is TRUE this returns FALSE.
     *  - If $Parent IsRoot is TRUE this returns TRUE.
     *
     * @param Path $Child The child path.
     * @param Path $Parent The parent path.
     * @retval boolean TRUE if $Child is a child of $Parent.
     */
    public static function IsChild( $Child,$Parent )
    {
        if( ($Child['IsRoot'] === TRUE && $Parent['IsRoot'] === TRUE) || ($Child['IsRoot'] === TRUE) )
            return FALSE;
        else if( $Parent['IsRoot'] === TRUE )
            return TRUE;

        return (count(array_intersect_assoc($Child['Segments'],$Parent['Segments'])) === count($Parent['Segments']));
    }

    /**
     * Add a segment to the end of the path.
     *
     * @param string $Element The segment to append.
     * @param Path $Subject The Path to append to.
     * @retval int The position, counting from 0, at which the element was appended.
     */
    public static function Append( $Element,&$Subject )
    {
        $Element = trim($Element);
        if( substr($Element,-1,1) === $Subject['Separator'] )
            $Subject['IsDir'] = TRUE;
        else
            $Subject['IsDir'] = FALSE;

        $Element = trim($Element,$Subject['Separator']);
        if( empty($Element) )
            return NULL;

        $Subject['Segments'] = $Subject['IsRoot']===TRUE?array():$Subject['Segments'];

        return parent::Append($Element,$Subject['Segments']);
    }

    /**
     * Add a segment to the beginning of the path.
     *
     * @param string $Element The segment to prepend.
     * @param Path $Subject The Path to prepend to.
     * @retval int 0
     */
    public static function Prepend( $Element,&$Subject )
    {
        $Element = trim($Element);

        // TODO: is this needed?  should we assume that the common case
        // is to not change whether the path is absolute
        if( substr($Element,0,1) === $Subject['Separator'] )
            $Subject['IsAbs'] = TRUE;

        $Element = trim($Element,$Subject['Separator']);
        if( empty($Element) )
            return NULL;

        $Subject['Segments'] = $Subject['IsRoot']===TRUE?array():$Subject['Segments'];

        return parent::Prepend($Element,$Subject['Segments']);
    }

    /**
     * Insert a new segment after another segment.
     *
     * @param string $Element The segment to insert.
     * @param int $RefPoint Reference point of insertion, starting from 0.
     * @param Path &$Subject The Path to insert into.
     */
    public static function InsertAfter( $Element,$RefPoint,&$Subject )
    {
        return parent::InsertAfter($Element,$RefPoint,$Subject['Segments']);
    }

    /**
     * Insert a new segment before another segment.
     *
     * @param string $Element The segment to insert.
     * @param int $RefPoint Reference point of insertion, starting from 0.
     * @param Path &$Subject The Path to insert into.
     */
    public static function InsertBefore( $Element,$RefPoint,&$Subject )
    {
        return parent::InsertBefore($Element,$RefPoint,$Subject['Segments']);
    }

    /**
     * Get a segment from the Path.
     *
     * See note for bottom - it may be appropriate to put ability to fetch
     * last, second-last, etc (perhaps modeled after substr().
     *
     * Right now it's a quick hack to mean a negative Needle to count in from the end.
     * No type of "length" is available (only one segment is returned).
     *
     * @param int $Needle Numeric index of segment, starting from 0.  A negative number will be counted from the end.
     * @param Path $Haystack The Path to read the segment from.
     * @param mixed $Check Optional value to check the segment against.
     */
    public static function Get( $Needle,$Haystack,$Check = NULL )
    {
        if( $Needle < 0 )
            return parent::Get(count($Haystack['Segments'])+$Needle,$Haystack['Segments'],$Check);
        else
            return parent::Get($Needle,$Haystack['Segments'],$Check);
    }

    /**
     * Prepend or append a segment in a Path.
     *
     * $Needle is a string defining the change to make:
     *  @li @c <segment: prepend the segment.
     *  @li @c >segment: append the segment.
     *
     * @param string $Needle Direction and segment to add.
     * @param array $Haystack Path Struct.
     * @retval int The position the segment was added.
     * @retval NULL Operation not recognized.
     *
     * @todo This may be expanded slightly.
     */
    public static function Set( $Needle,&$Haystack )
    {
        $Needle = trim($Needle);

        if( $Haystack['IsRoot'] === TRUE )
        {
            $Haystack['IsRoot'] = FALSE;
            $Haystack['Segments'][0] = ltrim(ltrim($Needle,'<'),'>');
            return count($Haystack['Segments']);
        }

        if( $Needle[0] === '<' )
            return static::Prepend(ltrim($Needle,'<'),$Haystack);
        else if( $Needle[0] === '>' )
            return static::Append(ltrim($Needle,'>'),$Haystack);
        else
            return NULL;
    }

    /**
     * Delete a segment by position.
     *
     * @param int $Needle The position of the segment, counting from 0.
     * @param array $Haystack Path Struct.
     */
    public static function Del( $Needle,&$Haystack )
    {
        return parent::Del($Needle,$Haystack['Segments']);
    }

    /**
     * Read one or more path segments from the top.
     *
     * In a path such as /one/two/three, "one" is the top most segment.
     *
     * @param array $Haystack Path Struct.
     * @param int $Limit Optional number of segments to read, starting from 1.
     * @retval string The single top-most path segment.
     * @retval array The specified number of top-most path segments as a new Path Struct.
     */
    public static function Top( $Haystack,$Limit = 0 )
    {
        if( $Limit > 0 )
        {
            $H2 = $Haystack;
            $H2['Segments'] = array_slice($Haystack['Segments'],0,$Limit);
            return $H2;
        }
        else
            return $Haystack['Segments'][0];
    }

    /**
     * Read one or more path segments from the bottom.
     *
     * In a path such as /one/two/three, "three" is the bottom most segment.
     *
     * @param array $Haystack Path Struct.
     * @param int $Limit Optional number of segments to read, starting from 1.
     * @retval string The single bottom-most path segment.
     * @retval array The specified number of bottom-most path segments as a new Path Struct.
     */
    public static function Bottom( $Haystack,$Limit = 0 )
    {
        if( is_string($Haystack) )
            $Haystack = self::Init($Haystack);

        if( $Limit > 0 )
        {
            $H2 = $Haystack;
            $H2['Segments'] = array_slice($Haystack['Segments'],count($Haystack['Segments'])-$Limit);
            return $H2;
        }
        else
            return $Haystack['Segments'][count($Haystack['Segments'])-1];
    }

    /**
     * Iterate through a Path segment by segment, left to right or right to left.
     *
     * This creates an array containing increasingly more or less specific versions of
     * the same path.
     *
     * By default this returns an array in increasing path size, i.e. most general to
     * most specific.
     *
     * @param array $Path The Path Struct.
     * @param boolean $Inc FALSE to iterate in decreasing path size, i.e. most specific to most general.
     * @retval array Ordered path segments.
     *
     * @note This doesn't honor IsDir or IsAbs of the Path struct - there will always be leading
     *       and trailing separators on all segments.
     */
    public static function Order( $Path,$Inc = TRUE )
    {
        if( Path::IsA($Path) === FALSE )
            throw new Exception('Path is not a Path Struct');

        if( $Path['IsRoot'] === TRUE )
            return array($Path['Separator']);

        $P = array();
        foreach( $Path['Segments'] as $K => $V )
            $P[] = ($K>0?$P[$K-1]:(($Path['Separator']))).$V.$Path['Separator'];

        if( $Inc === TRUE )
            return $P;
        else
            return array_reverse($P);
    }
}


/**
 * Tools for manipulating a hostname.
 *
 * The structure of a Hostname Struct is 0 through N subdomain parts,
 * counting from the TLD.
 *
 * For example www.asmblr.org would be represented as:
 *
 *  @li @c 0:  @c org
 *  @li @c 1:  @c asmblr
 *  @li @c 2:  @c www
 *
 * The Hostname::Bottom() method would return @c org and Hostname::Bottom()
 * would return @c www.
 *
 * This does not currently support IDNA, nor checks for invalid subdomain parts.
 *
 * @note Hostnames are "reversed" because DNS is "reversed".
 */
abstract class Hostname extends Struct
{
    /**
     * Create a new Hostname from a string.
     *
     * @param string $HostnameStr The hostname to parse.
     * @param string $Separator The subdomain separator or the period @c . by default.
     * @throws Exception HostnameStr not a string.
     * @retval array The Hostname Struct.
     *
     * @note The entire hostname is lowercased.
     */
    public static function Init( $HostnameStr,$Separator = '.' )
    {
        if( is_string($HostnameStr) === FALSE )
            throw new Exception('HostnameStr not a string.');

        if( $HostnameStr === '' )
            return static::$Skel;

        return array_reverse(explode($Separator,strtolower(trim($HostnameStr,'.'))));
    }

    /**
     * Create a string from the Hostname array.
     *
     * @param array $Hostname A Hostname Struct.
     * @retval string The hostname string.
     */
    public static function ToString( $Hostname )
    {
        return implode('.',array_reverse($Hostname));
    }

    /**
     * Add a subdomain to the "beginning" of a hostname.
     *
     * This remaps to Append().
     */
    public static function Prepend( $Element,&$Subject )
    {
        return parent::Append(trim(trim($Element),'.'),$Subject);
    }

    /**
     * Add a subdomain to the "end" of a hostname.
     *
     * This remaps to Prepend().
     */
    public static function Append( $Element,&$Subject )
    {
        return parent::Prepend(trim(trim($Element),'.'),$Subject);
    }

    /**
     * Prepend or append a subdomain in a Hostname.
     *
     * $Needle is a string defining the change to make:
     *  - \c <subdomain: prepend the subdomain.
     *  - \c >subdomain: append the subdomain.
     *
     * @param string $Needle Direction and subdomain to add.
     * @param array $Haystack Hostname Struct.
     * @retval int The position the subdomain was added.
     * @retval NULL Operation not recognized.
     *
     * @todo This may be expanded slightly.
     */
    public static function Set( $Needle,&$Haystack )
    {
        $Needle = trim($Needle);

        if( $Needle[0] === '<' )
            return static::Prepend(ltrim($Needle,'<'),$Haystack);
        else if( $Needle[0] === '>' )
            return static::Append(ltrim($Needle,'>'),$Haystack);
        else
            return NULL;
    }

    /**
     * Search for and return the position of a sub-domain.
     *
     * @param string $Needle The sub-domain to search for.
     * @param array $Hostname The Hostname Struct to search.
     * @retval int The position of the sub-domain.
     * @retval FALSE The sub-domain was not found.
     *
     * @note The search is case-sensitive.
     */
    public static function Search( $Needle,$Hostname )
    {
        return array_search($Needle,$Hostname);
    }

    /**
     * Read one or more sub-domains from the top.
     *
     * In a hostname such as www.asmblr.org, "org" is the top most sub-domain.
     *
     * @param array $Haystack Hostname Struct.
     * @param int $Limit Optional number of sub-domains to read, starting from 1.
     * @retval string The single top-most sub-domain.
     * @retval array The specified number of top-most sub-domains as a new Hostname Struct.
     */
    public static function Top( $Haystack,$Limit = 0 )
    {
        if( $Limit > 0 )
            return array_slice($Haystack,0,$Limit);
        else
            return $Haystack[0];
    }

    /**
     * Read one or more sub-domains from the bottom.
     *
     * In a hostname such as www.asmblr.org, "www" is the bottom most sub-domain.
     *
     * @param array $Haystack Hostname Struct.
     * @param int $Limit Optional number of sub-domains to read, starting from 1.
     * @retval string The single bottom-most sub-domain.
     * @retval array The specified number of bottom-most sub-domains as a new Hostname Struct.
     */
    public static function Bottom( $Haystack,$Limit = 0 )
    {
        if( $Limit > 0 )
            return array_slice($Haystack,count($Haystack)-$Limit);
        else
            return $Haystack[count($Haystack)-1];
    }


    /**
     * Iterate through a Hostname sub-domain by sub-domain, left to right or right to left.
     *
     * This creates an array containing increasingly more or less specific versions of
     * the same hostname.
     *
     * By default this returns an array in increasing hostname size, i.e. most general to
     * most specific.
     *
     * @param array $Hostname The Hostname Struct.
     * @param boolean $Inc Iterate in increasing hostname size, i.e., most general to most specific.
     * @retval array Ordered hostname sub-domains.
     *
     * @note Each hostname returned will have a leading period and no trailing period.
     */
    public static function Order( $Hostname,$Inc = TRUE )
    {
        $P = array();
        foreach( $Hostname as $K => $V )
            $P[] = ".{$V}".($K>0?$P[$K-1]:'');

        if( $Inc === TRUE )
            return $P;
        else
            return array_reverse($P);
    }
}


/**
 * Tools for manipulating a URL.
 *
 * The structure of a URL Struct is:
 *
 *  @li @c IsHTTPS: @c TRUE if the scheme is https.
 *  @li @c Scheme: Typically @c http or @c https.
 *  @li @c Username
 *  @li @c Password
 *  @li @c Hostname
 *  @li @c Port
 *  @li @c Path: A @c Path Struct
 *  @li @c Query: A @c URLEncoded Struct
 *  @li @c Fragment or @c #
 *
 * @note This does not support full URL/URI pedantics.
 */
abstract class URL extends Struct
{
    protected static $Skel = array('IsHTTPS'=>FALSE,'Scheme'=>'','Username'=>'','Password'=>'',
    							   'Hostname'=>array(),'Port'=>'','Path'=>array(),'Query'=>array(),'Fragment'=>'');


    /**
     * Create a new URL Struct from a string.
     *
     * Some URLs are indeterminate, such as a domain without a period or a filename without
     * an extension.  For example:
     *
     *  - @c domain.com
     *  - @c domain.com/
     *  - @c domain.com/something.html
     *  - @c /something.html
     *
     * @param string $URLStr The URL string to parse.
     * @throws Exception URLStr not a string.
     * @throws Exception Malformed URL '$URLStr' (parse_url()).
     * @retval array The URL Struct.
     *
     * @note This uses parse_url() and will prefix http:// if :// doesn't exist, and attempt detection
     *       of an absolute path (a period comes after a forward slash, if there is a forward slash).  If
     *       parse_url() cannot handle the URL, an exception is thrown.  Long story short, be mindful of
     *       what you're passing in and don't blindly rely on automatic detection of URLs vs paths.
     * @note Detection should work pretty well but domain.com vs filename.html isn't feasible so it's assumed a domain
     * @todo Set flag for returning based on merging current request with URLStr, akin to Request::CalcURLs
     */
    public static function Init( $URLStr )
    {
        $URL = static::$Skel;

        // empty string so return empty URL (just a root path with empty everything else)
        if( empty($URLStr) )
        {
            $URL['Path'] = Path::Init('/');
            return $URL;
        }

        if( is_string($URLStr) === FALSE )
            throw new Exception('URLStr not a string.');

        // perform some preprocess for domain vs path vs URL detection magic if there isn't a scheme
        if( strpos($URLStr,'://') === FALSE )
        {
            $p_p = strpos($URLStr,'.');
            $p_s = strpos($URLStr,'/');

            // a period before a slash, or no slash - prepend http://
            // domain.com  domain.com/  domain.com/something.html
            if( $p_p < $p_s || $p_s === FALSE )
            {
                $URLStr = "http://{$URLStr}";
            }
            // a slash before a period or slash at first character - treat as path only, leave hostname empty, and return
            // /something.html
            else if( $p_s < $p_p || $p_s === 0 )
            {
                $URL['Path'] = Path::Init($URLStr);
                $URL['Hostname'] = Hostname::Init('');
                $URL['Query'] = URLEncoded::Init('');
                return $URL;
            }
        }

        if( ($T = @parse_url(trim($URLStr))) === FALSE )
            throw new Exception("Malformed URL '$URLStr' (parse_url()).");

        $URL['Scheme'] = isset($T['scheme'])===TRUE?strtolower($T['scheme']):$URL['Scheme'];

        // If the scheme is https then it IsHTTPS = TRUE
        $URL['IsHTTPS'] = $URL['Scheme']==='https'?TRUE:$URL['IsHTTPS'];

        $URL['Hostname'] = isset($T['host'])===TRUE?Hostname::Init($T['host']):$URL['Hostname'];
        $URL['Port'] = isset($T['port'])===TRUE?$T['port']:$URL['Port'];

        $URL['Username'] = isset($T['user'])===TRUE?$T['user']:$URL['Username'];
        $URL['Password'] = isset($T['pass'])===TRUE?$T['pass']:$URL['Password'];

        $URL['Path'] = Path::Init(isset($T['path'])===TRUE?$T['path']:'');
        $URL['Query'] = isset($T['query'])===TRUE?URLEncoded::Init($T['query']):$URL['Query'];

        $URL['Fragment'] = isset($T['fragment'])===TRUE?$T['fragment']:$URL['Fragment'];

        return $URL;
    }

    /**
     * Create a new URL from individual parts.
     *
     * @param string $Scheme Typically http or https (:// is trimmed).
     * @param string $Hostname Hostname.
     * @param string|array $Path Path string or Path Struct.
     * @param string|array $Query Query string or URLEncoded Struct.
     * @param string|NULL $Port Optional port.
     * @param string|NULL $Username Optional username.
     * @param string|NULL $Password Optional password.
     * @param string|NULL $Fragment Optional fragment.
     * @retval array The URL.
     *
     * @note Explicitly setting a username/password in a URL may be incompatible with some browsers, like IE.
     */
    public static function InitParts( $Scheme,$Hostname,$Path,$Query,$Port = NULL,$Username = NULL,$Password = NULL,$Fragment = NULL )
    {
        $URL = static::$Skel;

        static::SetScheme($Scheme,$URL);
        static::SetHostname($Hostname,$URL);
        static::SetPath($Path,$URL);
        static::SetQuery($Query,$URL);

        if( $Port !== NULL )
            static::SetPort($Port,$URL);
        if( $Username !== NULL )
            static::SetUsername($Username,$URL);
        if( $Password !== NULL )
            static::SetPassword($Password,$URL);
        if( $Fragment !== NULL )
            static::SetFragment($Fragment,$URL);

        return $URL;
    }

    /**
     * Change the scheme of a URL.
     *
     * @param string $Scheme A string.
     * @param array $URL A URL Struct.
     *
     * @note This will affect the URL's IsHTTPS value.
     */
    public static function SetScheme( $Scheme,&$URL )
    {
        $URL['Scheme'] = strtolower(trim(trim($Scheme,'://')));
        $URL['IsHTTPS'] = $URL['Scheme']==='https'?TRUE:FALSE;
    }

    /**
     * Change the Hostname of a URL.
     *
     * @param string|array $Hostname A string or Hostname Struct to set.
     * @param array $URL A URL Struct.
     * @throws Exception Hostname not a string or Hostname Struct.
     */
    public static function SetHostname( $Hostname,&$URL )
    {
        if( is_string($Hostname) === TRUE )
            $URL['Hostname'] = Hostname::Init($Hostname);
        else if( is_array($Hostname) === TRUE )
            $URL['Hostname'] = $Hostname;
        else
            throw new Exception("Hostname not a string or Hostname Struct.");
    }

    /**
     * Change the Path of a URL.
     *
     * @param string|Path $Path A string or Path Struct to set.
     * @param array $URL A URL Struct.
     * @throws Exception Path not a string or Path Struct.
     */
    public static function SetPath( $Path,&$URL )
    {
        if( is_string($Path) === TRUE )
            $URL['Path'] = Path::Init($Path);
        else if( isset($Path['Segments']) === TRUE )
            $URL['Path'] = $Path;
        else if( $Path !== NULL )
            throw new Exception('Path not a string or Path Struct.');
    }

    /**
     * Change the Query of a URL.
     *
     * @param string|array $Query A string or URLEncoded Struct to set.
     * @param array $URL A URL Struct.
     * @throws Exception Query not a string or URLEncoded Struct.
     */
    public static function SetQuery( $Query,&$URL )
    {
        if( is_string($Query) === TRUE )
            $URL['Query'] = URLEncoded::Init($Query);
        else if( is_array($Query) === TRUE )
            $URL['Query'] = $Query;
        else if( $Query !== NULL )
            throw new Exception("Query not a string or URLEncoded Struct.");
    }

    /**
     * Change the Port of a URL.
     *
     * @param string|int $Port A string or integer.
     * @param URL $URL A URL Struct.
     *
     * @note The port is always converted to a string.
     * @note If the port is 80 or 443, it is set as the empty string.
     */
    public static function SetPort( $Port,&$URL )
    {
        if( in_array((string)$Port,array('','80','443')) === TRUE )
            $URL['Port'] = '';
        else
            $URL['Port'] = (string) $Port;
    }

    /**
     * Change the Username of a URL.
     *
     * @param string $Username A string.
     * @param array $URL A URL Struct.
     *
     * @note Explicitly setting a username in a URL may be incompatible with some browsers, like IE.
     */
    public static function SetUsername( $Username,&$URL )
    {
        $URL['Username'] = $Username===NULL?$URL['Username']:$Username;
    }

    /**
     * Change the Password of a URL.
     *
     * @param string $Password A string.
     * @param array $URL A URL Struct.
     *
     * @note A Username check is not done.
     * @note Explicitly setting a password in a URL may be incompatible with some browsers, like IE.
     */
    public static function SetPassword( $Password,&$URL )
    {
        $URL['Password'] = $Password===NULL?$URL['Password']:$Password;
    }

    /**
     * Change or delete the Fragment of a URL.
     *
     * @param string $Fragment Fragment or a single # to delete an existing one.
     * @param array $URL A URL Struct.
     */
    public static function SetFragment( $Fragment,&$URL )
    {
        if( $Fragment === '#' )
            $URL['Fragment'] = '';
        else if( $Fragment !== NULL )
            $URL['Fragment'] = ltrim($Fragment,'#');
    }

    /**
     * Create a string from the URL array, depending on the URL parts present.
     *
     * @param array $URL The URL array.
     * @throws Exception URL is not URL Struct.
     * @retval string URL string.
     *
     * @note Logic exists to handle an empty hostname in which case scheme/port/username/password isn't included
     *       and thus a path-only "URL" is returned.
     * @note Explicitly setting a username in a URL may be incompatible with some browsers, like IE.
     * @todo At some point we may use the HTTP PECL,
     * 		 http://us2.php.net/manual/en/function.http-build-url.php
     */
    public static function ToString( $URL )
    {
        $Str = '';
        if( !empty($URL['Hostname']) )
        {
            $Str = $URL['Scheme']!==''?"{$URL['Scheme']}://":'';
            if( !empty($URL['Username']) )
                $Str .= static::AuthorityToString($URL['Username'],$URL['Password']);

            $Str .= Hostname::ToString($URL['Hostname']).($URL['Port']===''?'':":{$URL['Port']}");
        }

        // if a hostname is present, ensure a / for relative paths - otherwise use what Path has
        $Str .= (!empty($Str)&&empty($URL['Path']['IsAbs'])?'/':'').Path::ToString($URL['Path'],'url');
        $Str .= empty($URL['Query'])?'':URLEncoded::ToString($URL['Query']);
        $Str .= $URL['Fragment']===''?'':'#'.rawurlencode($URL['Fragment']);

        return $Str;
    }

    /**
     * Change parts of a URL Struct.
     *
     * $Needle is a change string or array of strings defining the changes to make:
     *  - @c >segment: append a segment to the path.
     *  - @c <segment: prepend a segment to the path.
     *  - @c ?key=value: set a key/value in the query string.
     *  - @c ?key=: delete a key/value in the query string.
     *  - @c ?: delete entire query string.
     *  - @c \#fragment: set the URL fragment.
     *  - @c #: delete the URL fragment.
     *
     * $Needle may also combine segment, ?key=value and \#fragment change strings:
     *  - \c >new-segment?login=1&register=&redirect=1
     *
     * This would append a new path segment, set login and redirect query variables
     * to 1, and remove the register variable.
     *
     * The array syntax for URLEncoded::Set() is also supported.
     *
     * @param string $Needle A change to make.
     * @param array $Needle An array of changes to make.
     * @param array &$Haystack URL Struct.
     *
     * @todo This needs optimization and perhaps better support, including appending/prepending
     *       more than one path segment at a time (i.e. from LinkPage).
     */
    public static function Set( $Needle,&$Haystack )
    {
        foreach( (array) $Needle as $K => $V )
        {
            if( is_array($V) )
            {
                URLEncoded::Set($V,$Haystack['Query']);
                continue;
            }
            else if( !is_int($K) )
            {
                URLEncoded::Set(array(array($K=>$V)),$Haystack['Query']);
                continue;
            }

            $V2 = explode('#',$V);
            if( isset($V2[1]) )
                static::SetFragment($V2[1],$Haystack);

            $V2 = explode('?',$V2[0]);
            if( isset($V2[1]) )
                URLEncoded::Set($V2[1],$Haystack['Query']);

            if( !empty($V2[0]) )
            {
                if( ($V2[0][0] === '>' || $V2[0][0] === '<') )
                {
                    Path::Set($V2[0],$Haystack['Path']);
                    $Haystack['Path']['IsAbs'] = TRUE;
                }
                else
                {
                    trigger_error("URL::Set() Unrecognized change string '{$V}'");
                }
            }
        }
    }

    /**
     * Read the full hostname as a string.
     *
     * @param array $URL URL Struct.
     * @retval string The hostname string.
     */
    public static function Hostname( $URL )
    {
        return Hostname::ToString($URL['Hostname']);
    }

    /**
     * Read the full path as a string.
     *
     * @param array $URL URL Struct.
     * @retval string The path string.
     */
    public static function Path( $URL )
    {
        return Path::ToString($URL['Path'],'url');
    }

    /**
     * Helper for creating a URL authority part from a username and password.
     *
     * @param string|NULL $Username The username, an empty string, or NULL.
     * @param string|NULL $Password The password, an empty string, or NULL.
     * @retval string URL authority string, which may be an empty string.
     *
     * @note A password without a username will return an empty string.
     * @note Explicitly setting a username in a URL may be incompatible with some browsers, like IE.
     */
    public static function AuthorityToString( $Username,$Password )
    {
        if( $Username !== NULL && $Username !== '' )
            return rawurlencode($Username).($Password!==''&&$Password!==NULL?':'.rawurlencode($Password).'@':'@');
        else
            return '';
    }
}


/**
 * Tools for manipulating encoded key/value pairs, such as GET query strings and POST data.
 *
 * The structure of a URLEncoded Struct is that of one or more key/value pairs.
 *
 * @note This should not be used for multipart/form-data (PHP_QUERY_RFC1738).
 * @note This uses http_build_query() with PHP_QUERY_RFC3986 (rawurlencode()).
 *
 * @todo Provide functionality for appending/combining key/value pairs before/after one another.
 */
abstract class URLEncoded extends Struct
{
    /**
     * Create a new URLEncoded from a string.
     *
     * @param string $URLEncStr The URL encoded string to parse.
     * @retval array The URLEncoded.
     * @throws Exception URLEncStr not a string.
     *
     * @note This uses parse_str().
     */
    public static function Init( $URLEncStr )
    {
        if( is_string($URLEncStr) === FALSE )
            throw new Exception('URLEncStr not a string.');

        $Q = array();
        parse_str($URLEncStr,$Q);
        return $Q;
    }

    /**
     * Add or remove key/values in a URLEncoded Struct.
     *
     * $Needle is change string, an array of change strings, or an array
     * of associative array key/value pairs, defining the changes to make:
     *  - @c ?key=value&key1=value1: set the key/value pairs from a string.  setting empty will delete the key.
     *  - @c ?: remove all key/values
     *  - @c array('key'=>'value'): set the key/value from an array
     *  - @c array('key'=>NULL): remove the key/value, if it exists
     *  - @c array(): remove all key/values
     *
     * @param array $Needle An array of key/values as strings or arrays.
     * @param string $Needle A key=value change string.
     * @param array &$Haystack URLEncoded Struct.
     * @retval void
     */
    public static function Set( $Needle,&$Haystack )
    {
        foreach( (array) $Needle as $K => $V )
        {
            if( empty($V) || $V === '?' )
            {
                $Haystack = array();
                continue;
            }

            if( is_string($V) )
            {
                parse_str(ltrim($V,'?'),$V2);
            }
            else if( is_array($V) )
            {
                $V2 = $V;
            }

            foreach( $V2 as $I => $J )
            {
                if( isset($Haystack[$I]) && empty($J) )
                    static::Del($I,$Haystack);
                else
                    $Haystack[$I] = $J;
            }
        }
    }

    /**
     * Create a string from the URLEncoded Struct.
     *
     * @param array $URLEncoded Array of key/value pairs.
     * @param string $Prefix Optional string prefix.
     * @retval string The URL encoded string.
     *
     * @note This uses http_build_query() with the PHP_QUERY_RFC3986 encode type.
     *
     * @todo And one or more levels of recursion, which would flatten into a single string, whereas
     * 		 http_build_query() currently keeps them multi-dimensional.
     */
    public static function ToString( $URLEncoded,$Prefix = '?' )
    {
        return ($Tmp = http_build_query($URLEncoded,'','&',PHP_QUERY_RFC3986))===''?'':"{$Prefix}{$Tmp}";
    }
}


/**
 * Base class to encapsulate key/value pairs and their getter/setter logic
 * for local memory-resident variables.
 *
 * A KeyValueSet (KVS) is a data structure and group of methods for iteration, getters,
 * setters, etc.
 */
class KeyValueSet implements KVS,Directable
{
    /**
     * @var array $KV
     * Key/value pairs that comprise the KVS's data.
     */
    protected $KV = array();

    /**
     * @var int $KVLength
     * The number of key/value pairs in this KVS.
     *
     * @note This and {@link $KVPosition} are used in a just-in-time fashion for iteraion.
     * 		 They are not kept current during get/set/unset operations.
     */
    protected $KVLength = 0;

    /**
     * @var int $KVPosition
     * The current position within KeyValueSet::$KVLength.
     */
    protected $KVPosition = 0;


    /**
     * Create a new KVS object.
     *
     * If ArrayRef isn't supplied, the KVS will reference it's own empty array.
     *
     * @param array|NULL &$ArrayRef Reference to an array which contains the KV's key/value pairs.
     * @throws KV must reference an array, TYPE given.
     */
    public function __construct( &$ArrayRef = NULL )
    {
        if( is_array($ArrayRef) === TRUE )
            $this->KV = &$ArrayRef;
        else if( $ArrayRef !== NULL )
            throw new Exception('KV must reference an array, '.gettype($ArrayRef).' given.');
    }

    /**
     * Key/value getter using property overloading.
     *
     * @param integer|string $Key The element's key to get.
     * @retval mixed|NULL The element's value or NULL if $Key isn't __isset().
     */
    public function __get( $Key )
    {
        return $this->__isset($Key)===TRUE?$this->KV[$Key]:NULL;
    }

    /**
     * Key/value setter using property overloading.
     *
     * If $Key exists, it's value is silently overwritten.
     *
     * @param integer|string $Key The element's key to set.
     * @param mixed $Value The element's value to set.
     */
    public function __set( $Key,$Value )
    {
        $this->KV[$Key] = $Value;
    }

    /**
     * Determine if a key is set using property overloading.
     *
     * For performance, isset() is first used to check whether $Key
     * is set (non-NULL).  This check is case-sensitive and loosely typed.
     *
     * If isset() returns FALSE, array_key_exists() is used
     * to check if $Key is set, but is set to NULL.  This check is
     * case-sensitive and loosley typed.
     *
     * @param integer|string $Key The element's key to check.
     * @retval boolean TRUE if the $Key exists.
     *
     * @note Tempting PHP's loosely typed guesswork can lead to painfully subtle
     * 	     bugs - use only non-numeric strings or actual integers as keys.
     */
    public function __isset( $Key )
    {
        return (isset($this->KV[$Key])===TRUE?TRUE:array_key_exists($Key,$this->KV));
    }

    /**
     * Unset a key/value pair using property overloading.
     *
     * @param integer|string $Key The element's key to unset.
     * @retval boolean TRUE if $Key was found.
     */
    public function __unset( $Key )
    {
        if( $this->__isset($Key) === TRUE )
        {
            $this->KV[$Key] = NULL;
            unset($this->KV[$Key]);
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    /**
     * @retval integer The number of key/value pairs.
     * @note Implement SPL's Countable interface.
     */
    public function count(): int
    {
        return count($this->KV);
    }

    /**
     * @note Implement SPL's Iterator interface.
     * @note This is where KeyValueSet::$KVPosition and KeyValueSet::$KVLength are initialized.
     */
    public function rewind(): void
    {
        $this->KVLength = count($this->KV);
        $this->KVPosition = 0;
        reset($this->KV);
    }

    /**
     * @retval mixed
     * @note Implement SPL's Iterator interface.
     * @note Use __get() in extending classes to maintain the processing behavior it does.
     */
    public function current()
    {
        return $this->__get(key($this->KV));
    }

    /**
     * @retval mixed
     * @note Implement SPL's Iterator interface.
     */
    public function key()
    {
        return key($this->KV);
    }

    /**
     * @note Implement SPL's Iterator interface.
     * @note Uses {@link $KVPosition}.
     */
    public function next(): void
    {
        ++$this->KVPosition;
        next($this->KV);
    }

    /**
     * @retval boolean
     * @note Implement SPL's Iterator interface.
     * @note Uses {@link $KVPosition} and {@link $KVLength}.
     */
    public function valid(): bool
    {
        return ($this->KVPosition < $this->KVLength);
    }

    /**
     * @retval mixed
     * @note Implement SPL's ArrayAccess interface.
     */
    public function offsetGet( $Key )
    {
        return $this->__get($Key);
    }

    /**
     * @note Implement SPL's ArrayAccess interface.
     */
    public function offsetSet( $Key,$Value ): void
    {
        $this->__set($Key,$Value);
    }

    /**
     * @retval boolean
     * @note Implement SPL's ArrayAccess interface.
     */
    public function offsetExists( $Key ): bool
    {
        return $this->__isset($Key);
    }

    /**
     * @note Implement SPL's ArrayAccess interface.
     */
    public function offsetUnset( $Key ): void
    {
        $this->__unset($Key);
    }

    /**
     * Implement Directable interface.
     *
     * This allows key/value pairs to be set via manifest directives.
     *
     * @param string $Key The name of the key to set.
     * @param mixed $Value The value to set.
     */
    public function ApplyDirective( $Key,$Value )
    {
        $this->KV[$Key] = $Value;
    }

    /**
     * Set the KVS to reference another array.
     *
     * @param $ArrayRef A reference to an array.
     */
    public function Import( &$ArrayRef = NULL )
    {
        $this->KV = &$ArrayRef;
    }

    /**
     * Return the referenced array.
     * @retval array
     */
    public function Export()
    {
        return $this->KV;
    }
}

