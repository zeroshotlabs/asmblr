<?php
/**
 * @file Page.inc URL matching and application execution.
 * @author Stackware, LLC
 * @version 4.2
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * A Page represents a unit of application logic for a Path or hierarchy of Paths.
 *
 * A Page contains one trigger absolute Path, a Name, an optional function, and none, one or more
 * Directives.  The Path and Name must be unique within a PageSet.
 *
 * Pages are usually executed and managed by a PageSet, forming the "control layer".  PageSets and
 * Pages are managed through the manifest.
 */
abstract class Page extends Struct
{
    protected static $Skel = array('Name'=>'','Path'=>'','Status'=>'','PathStruct'=>array(),'Function'=>array(),'Directives'=>array());


    /**
     * Create a new Page Struct.
     *
     * @param string $Name The name of the Page.
     * @param string $Path The trigger absolute path of the Page which is lowercased.
     * @param string $Status The page's status, generally @c Active.
     * @param string|array $Function A function callback.
     * @retval array The created Page Struct.
     *
     * @note The Path should not be encoded.
     */
    public static function Init( $Name,$Path,$Status,$Function = NULL )
    {
        $Page = static::$Skel;

        $Page['Name'] = $Name;

        $Page['Path'] = strtolower($Path);
        $Page['PathStruct'] = Path::Init($Page['Path']);

        $Page['Status'] = $Status;

        $Page['Function'] = $Function;

        return $Page;
    }
}


/**
 * A set of Pages used for application execution control.
 *
 * A PageSet manages, stores, matches and executes Pages.
 *
 * Page Names and Paths are unqiue and overwritten silently.
 *
 * This is the default execution and request routing mechanism in asmblr.
 */
class PageSet implements Debuggable
{
    use Debugged;

    /**
     * @var boolean Whether to display debug info to the browser.
     * @see PageSet::DebugOn()
     */
    protected $DebugDisplay = FALSE;

    public $Pages;
    public $PageMap;


    /**
     * Create a PageSet.
     *
     * @param array $Pages Numeric index of Page Struct to be managed.
     * @param array $PageMap Mapping of Page paths and the page name.
     */
    public function __construct( $Pages,$PageMap )
    {
        $this->Pages = $Pages;
        $this->PageMap = $PageMap;
    }

    /**
     * Lookup a Page by it's absolute Path.
     *
     * An exact match of a Page's Path is done.  Only one or none Pages will match.
     *
     * The matching Page Struct will be set in PageSet::$Matched (using Page Name) and returned.
     *
     * @param string $Path The absolute path to match.
     * @retval array The matched Page Struct.
     * @retval NULL No Page matched.
     *
     * @note This is the primary Page routing mechanism in asmblr.
     * @note Matching should be done against unencoded paths.
     * @note If multiple pages with the same name are matched (rare), only the first instance will be stored in $Matched.
     */
    public function Match( $Path )
    {
        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugMatch($Path);

        return (isset($this->PageMap[$Path])===TRUE?$this->Pages[$this->PageMap[$Path]]:NULL);
    }

    /**
     * Execute a Page.
     *
     * Execution of a Page entails:
     *  - Appying Directives, if any
     *  - Executing the Function, if any
     *
     * The executing Page Struct will be set in PageSet::$Executed (using Page Name).
     *
     * @param array $Page The Page Struct to execute.
     * @retval mixed The value returned by the function, if any.
     * @throws Exception Directive object ... doesn't exist while executing Page ...
     *
     * @note It is not enforced that the Page must exist in the PageSet, i.e. this can be used to execute any Page.
     * @note If multiple pages with the same name are executed, only the first instance will be stored in $Executed.
     */
    public function Execute( $Page )
    {
        global $asmapp;

        if( isset($_SERVER[$this->DebugToken]) )
        {
            $BT = Debug::Backtrace();
            $BT = current(Debug::BT2Str($BT,'Execute'));

            Log::Log("\${$this->DebugToken}::Execute('{$Page['Name']}') called from {$BT}",'WARN',NULL,$Page);
        }

        // would like to streamline this somehow
        foreach( $Page['Directives'] as $V )
        {
            if( empty($asmapp->$V[0]) )
                throw new Exception("Directive object '{$V[0]}' doesn't exist while executing Page '{$Page['Name']}'.");
            else
                $asmapp->$V[0]->ApplyDirective($V[1],$V[2]);
        }

        if( !empty($Page['Function']) )
        {
            // this automatically executes class::func when an array... sweet
            return $Page['Function']($asmapp);
        }
    }

    /**
     * Enable debugging, overridding default Label behavior.
     *
     * Log messages are always labeled with the class name.
     *
     * @param boolean $Label TRUE to display debug info into rendering templates.
     */
    public function DebugOn( $Label = NULL )
    {
        $this->DebugToken = get_class($this);
        $_SERVER[$this->DebugToken] = TRUE;

        if( $Label === TRUE )
            $this->DebugDisplay = TRUE;
    }

    /**
     * Internal method for debugging matching a page.
     *
     * @param string $Path Path that's matched.
     */
    protected function DebugMatch( $Path )
    {
        $BT = Debug::Backtrace();
        $BT = current(Debug::BT2Str($BT,'Match'));

        if( isset($this->PageMap[$Path]) )
        {
            $P = $this->Pages[$this->PageMap[$Path]];
            $Buf = "\${$this->DebugToken}::Match('{$Path}') to '{$P['Name']}' called from {$BT}";
        }
        else
        {
            $Buf = "\${$this->DebugToken}::Match('{$Path}') to NO MATCH called from {$BT}";
        }

        if( !empty($this->DebugDisplay) )
            echo "<div style=\"padding: 0; color: blue; margin: 0; font-weight: bold; font-size: 10px;\">$Buf</div>";

        Log::Log($Buf,'LOG',NULL,NULL);
    }
}

