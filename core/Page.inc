<?php
/**
 * @file Page.inc URL matching and logic execution.
 * @author Stackware, LLC
 * @version 4.0
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
 * Pages are usually executed and managed by a PageSet, together loosely forming
 * Framewire's MVC <b>c</b>ontrol layer.
 *
 * @see PageSet
 */
abstract class Page extends Struct
{
    protected static $Skel = array('Name'=>'','Path'=>'','Status'=>'','PathStruct'=>array(),'Function'=>array(),'Directives'=>array());


    /**
     * Create a new Page Struct.
     *
     * @param string $Name The name of the Page.
     * @param string $Path The trigger absolute path of the Page which is lowercased.
     * @param string $Routine A Routine string; both inline or pointer are accepted.
     * @param string|array $Directives A single Directive definition or an array of them.
     * @throws Exception Invalid Page Name '$Name'.
     * @retval array The created Page Struct.
     *
     * @note The Path should be absolute and always directly off the root.  It should not
     *       contain any prefixes as these are handled automatically using AppURL.
     * @note The Path should not be encoded.
     * @todo Passing in Directives (here) may be deprecated.
     */
    public static function Init( $Name,$Path,$Status,$Function = NULL,$Directives = NULL )
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
 * This is the default execution and request routing mechanism in Framewire.
 */
class PageSet implements Debuggable
{
    use Debugged;

    public $Pages;
    public $PageMap;


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
     * @note This is the primary Page routing mechanism in Framewire.
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
     * @throws Exception Directive Name {$V['Name']}' doesn't exist while executing Page '{$Page['Name']}'.
     *
     * @todo We'd love to optimize/streamline applying directives somehow.
     *
     * @note It is not enforced that the Page must exist in the PageSet,
     *       i.e. this can be used to execute any Page.
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
            // this automatically executes class::func when an array..??? sweet
            $Page['Function']($asmapp);
        }
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

        Log::Log($Buf,'LOG',NULL,NULL);
    }
}

