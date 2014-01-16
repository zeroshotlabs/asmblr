<?php
/**
 * @file App.inc asmblr application controller.
 * @author Stackware, LLC
 * @version 4.0
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * Base configuration, controller and runtime container for an application.
 *
 * It should be extended and customized in index.php.
 *
 * @note This is your GOD anti-pattern, including dynamic properties.  The
 *       mess goes here, so the rest is clean.
 */
abstract class App
{
    public $Request;
    public $Config;
    public $Directives;
    public $Pages;
    public $PathPageMap;
    public $Templates;


    public function __construct( $Request,$Manifest )
    {
        $this->Request = $Request;
        $this->Manifest = $Manifest;

        $this->Config = &$this->Manifest['Config'];

        $this->Directives = &$this->Manifest['Directives'];

        $this->Pages = &$this->Manifest['Pages'];
        $this->PathPageMap = &$this->Manifest['PathPageMap'];

        $this->Templates = &$this->Manifest['Templates'];

        // Put this object global
        $GLOBALS['asmapp'] = $this;
    }


    /**
     * Generic handler for when no pages could be matched.
     *
     * This method should be overridden in the application's extending App class.
     */
    public function NoPageHandler()
    {
        Log::Log('NO PAGE FOUND: '.URL::ToString($this->Request));

        if( $this->Request['IsCLI'] === FALSE )
            \asm\HTTP::_404();

        exit;
    }


    /** The handlers below could be a recursion danger zone.  :) **/

    /**
     * User error handler.
     *
     * Handle an application or PHP error.  This method is set using
     * set_error_handler() in App::__construct().
     *
     * Framewire uses PHP's native error handling, allowing well-known
     * PHP functions such as trigger_error() to intelligently deliver
     * error messages using advanced mechanisms, such as Wildfire or
     * email.
     *
     * This method maps PHP's error constants to Framewire's Log error
     * levels.
     *
     * @param int $errno The message severity as a PHP constant.
     * @param string $errstr The error message.
     * @param string $errfile The filename from which the message came.
     * @param int $errline The line number of the file.
     * @param array $errcontext Local scope where the error occurred.
     * @retval void
     *
     * @see FatalErrorHandler()
     * @todo errcontext can be huge - might filter/etc
     */
    public function ErrorHandler( $errno,$errstr,$errfile,$errline,$errcontext )
    {
        // error surpressed with @
        if( error_reporting() === 0 )
            return FALSE;

        if( in_array($errno,array(E_WARNING,E_USER_WARNING)) )
            $errno = 'WARN';
        else if( in_array($errno,array(E_NOTICE,E_USER_NOTICE)) )
            $errno = 'INFO';
        else
            $errno = 'ERROR';

        $BT = array_merge(array("[{$errfile}:{$errline}]"),Debug::Backtrace());

        Log::Log($errstr,$errno,$BT,$errcontext);

        return TRUE;
    }

    /**
     * Handle fatal and internal PHP errors.
     *
     * This method is set using register_shutdown_function() in App::__construct()
     * (so technically it's a shutdown function, not an error handler).
     *
     * It allows most fatal/parse errors to be handled in the same way as user
     * errors are by Framewire.
     *
     * @note Cannot handle fatal errors that occur in fwboot.php
     * @note This could go recursive if any errors occur in ErrorHandler() or Log.
     * @note PHP bug: in certain cases, like trying to consecutively include() two files that don't exist,
     *       this is called as a fatal error, but there is no way to detect whether was @ was used -
     *       see Instance::__construct when CacheApp === TRUE
     */
/*
    public function FatalErrorHandler()
    {
        if( ($E = error_get_last()) !== NULL )
        {
            $this->ErrorHandler($E['type'],$E['message'],$E['file'],$E['line'],NULL);
        }
    }
*/

    /**
     * Handle uncaught exceptions.
     *
     * This method is set using set_exception_handler() in App::__construct().
     *
     * This method should be overridden in the application's extending App class.
     *
     * @param Exception $E An uncaught exception.
     */
/*
    public function UncaughtExceptionHandler( \Exception $E = NULL )
    {
        Log::Log('UNCAUGHT '.(string)$E,'ERROR',$E->getTrace());
    }
*/
    /**
     * Set PHP's open_basedir() directive for the application.
     *
     * @param string $Path Base directory.
     * @todo implement/test
     */
    public function OpenBaseDir( $Path = '' )
    {
        if( empty($Path) )
        {
            // these defaults need work - should use cache directory and AppRoot or something
            // so no this is a no op
            return;

            if( Instance::IsWindows() )
                ini_set('open_basedir',ASM_ROOT.';C:/Windows/Temp/');
            else
                ini_set('open_basedir',ASM_ROOT.':/tmp');
        }
        else
            ini_set('open_basedir',$Path);
    }
}
