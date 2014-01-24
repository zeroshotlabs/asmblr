<?php
/**
 * @file Template.inc Text templating and rendering.
 * @author Stackware, LLC
 * @version 4.0
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * A Template is a block of text that can be rendered.
 *
 * A Template contains a Name, and optional function, and a body.  The Name
 * must be unique within a TemplateSet.
 *
 * If a function is present, it is executed before rendering occurs.  If the
 * function returns FALSE, rendering will not occur.
 *
 * Templates are usually rendered and managed by a TemplateSet, together
 * loosely forming Framewire's MVC <b>v</b>iew layer.
 *
 * @see TemplateSet
 */
abstract class Template extends Struct
{
    protected static $Skel = array('Name'=>'','Function'=>array(),'Path'=>'','Body'=>'');


    /**
     * Create a new Template Struct.
     *
     * @param string $Name The name of the Template
     * @param string $Path The filesystem path of the template's content.
     * @param string $Function A Function array
     * @param string $Body The contents of the Template.
     * @throws Exception Template Body not a string with '$Name'.
     * @retval array The created Template Struct.
     *
     * @note A template function should be prototyped function( $this->Connected,$Args,$App )
     */
    public static function Init( $Name,$Path,$Function = array(),$Body = '' )
    {
        $Template = static::$Skel;

        $Template['Name'] = $Name;
        $Template['Path'] = $Path;

        if( is_string($Body) === FALSE && empty($Body) === FALSE )
            throw new Exception("Template Body not a string with '$Name'.");

        $Template['Body'] = (string) $Body;

        $Template['Function'] = $Function;

        return $Template;
    }
}


/**
 * A set of Templates used for rendering and output of text.
 *
 * A TemplateSet manages, renders and outputs Templates.
 *
 * Template Names are unique and overwritten silently.
 *
 * This is the default templating mechanism in Framewire.
 *
 * This TemplateSet manages file-based templates.
 */
class TemplateSet implements Debuggable
{
    use Debugged;

    protected $App;

    /**
     * Array of managed Template Structs.
     *
     * Current these will be Template structs with no body in dev/non-cache mode.
     * Once production/cache mode is implemented, these may contain the body too.
     */
    protected $Templates = array();

    /**
     * Array of Wired()'d objects and arrays that are made
     * available within all templates using Connect().
     */
    protected $Connected = array();

    /**
     * Cache of the last generated Connect()'d variables, suitable for pushing
     * into the scope of a rendering template.
     *
     * @todo Consider other options as this has little gain.
     */
    protected $ConnectedScope = array();

    /**
     * Contains Template stacks for grouped rendering.
     * @see Stack();
     * @see Unstack();
     */
    protected $Stacks = array();



    public function __construct( \asm\App $App )
    {
        $this->App = $App;
        // may change
        $this->Templates = $App->Templates;
    }

    /**
     * Check if a Name will render when called.
     *
     * @param string $Name A Name or ReMap()'d Name.
     * @retval boolean TRUE if the Name exists and is not a NULL render.
     */
    public function __isset( $Name )
    {
        return isset($this->Templates[$Name]);
    }

    /**
     * Map a Name to another Template Name.
     *
     * This is a shorthand for ReMap().
     *
     * @param string $Name The Name to map.
     * @param string $DestName The Template to map to.
     *
     * @see ReMap()
     */
    public function __set( $Name,$DestName )
    {
        $this->ReMap($Name,$DestName);
    }

    /**
     * Render and output a Template that exists in this TemplateSet.
     *
     * The rendered Template is outputted directly.
     *
     * Rendering begins with the named Template.  The Template Name is resolved
     * as follows:
     *  - The Name is checked for in $ReMapped.  If it's been ReMap()'d to another
     *    Name, then the Template with that Name is rendered.  If it's been
     *    ReMap()'d to NULL, nothing is rendered.
     *  - The Name is checked for in $Templates and if found that Template is rendered.
     *
     * If the rendering Template has a Routine defined, it will executed prior to
     * rendering.  For Pointer Routines, if FALSE is returned, rendering will not occur.
     * An Inline Routine will be locally scoped and should set $AllowRender to FALSE
     * to prevent rendering.  It can also access the Template Struct directly as
     * $RenderingTemplate.
     *
     * This method can be called with an associative array.  Each element of the array
     * will be made available as a variable in the Template's Body during rendering.
     *
     * <?php
     *   $Data = array('FirstName'=>'John','LastName'=>'Doe','Items'=>array('One','Two'));
     *   $this->Header($Data);
     * ?>
     *
     * This will create three variables - $FirstName, $LastName and $Items - in the
     * Template Body's scope during rendering.
     *
     * All Templates will also have access to variables that have been Connect()'d,
     * and to the TemplateSet object itself, available as $this.
     *
     * While combining variables from multiple sources is convenient, it is important
     * to consider that naming conflicts can occur.  To avoid confusion, remember the
     * ordering in which variables are pushed into a Template's scope, thus overwritting
     * one another:
     *  1. Connect()'d variables, using their Connect()'d Name.
     *  2. Render-time arguments.
     *  3. The TemplateSet itself as $this.
     *
     * @param string $Name A ReMap()'d Name or Template Name to render.
     * @param array $Args Optional associative array of keys/values variables for the Template's scope.
     *
     * @note This uses eval() to do the rendering.
     * @note Render functions cannot ReMap() the template they are executing under, though they can
     *       manually render another template and then return FALSE.
     * @note Render functions should expect $this->Connected,$Args,$this->App as parameters.
     * @note Variables $RenderingTemplate, $AllowRender and $this are reserved variable names.
     * @note WARNING: There is no escaping - it should be done inline in the Template's Body.
     * @note No error is raised if the Template Name isn't found and debugging is off.
     *
     * @todo Test the behavior of multiple keys/vales in $Args.
     * @todo Test AllowRender behavior for both Pointer/Inline routines.
     */
    public function __call( $Name,$Args )
    {
        if( isset($_SERVER[$this->DebugToken]) )
            $this->Debug__call($Name,$Args);

        if( isset($this->Templates[$Name]) )
        {
            $RenderingTemplate = $this->Templates[$Name];

            // scope the connected variables plus any arguments
            foreach( $this->Connected as $K => $V )
                $$K = $V;

            if( !empty($Args[0]) )
            {
                foreach( $Args[0] as $K => $V )
                    $$K = $V;
            }

            if( empty($RenderingTemplate['Function']) )
            {
                // if empty body, assume we have the body on the filesystem
                // otherwise eval() it (basically just to support @@@)
                // TODO: sort of clumsy...???
                if( empty($RenderingTemplate['Body']) )
                    include $RenderingTemplate['Path'];
                else
                    eval("?>{$RenderingTemplate['Body']}");
            }
            else
            {
                // the function should expects arguments $this->Connected,$Args,$App
                if( ($RenderingTemplate['Function']($this->Connected,$Args,$this->App)) !== FALSE )
                {
                    if( empty($RenderingTemplate['Body']) )
                        include $RenderingTemplate['Path'];
                    else
                        eval("?>{$RenderingTemplate['Body']}");
                }
            }
        }
    }

    /**
     * Render a Template, returning it's output by default.
     *
     * Helper function for rendering Templates.
     *
     * @param string $Name A ReMap()'d Name or Template Name to render.
     * @param boolean $Return FALSE for direct output.
     * @retval string The rendered Template.
     *
     * @note Passing arguments is not supported.
     * @see __call()
     * @todo Add/test $Args support.
     */
    public function Render( $Name,$Return = TRUE )
    {
        if( $Return === TRUE )
        {
            ob_start();
            $this->$Name();
            return ob_get_clean();
        }
        else
        {
            $this->$Name();
        }
    }

    /**
     * Map a Name to another Template Name.
     *
     * This will cause $DestName to be rendered when $Name is called.
     *
     * @param string $Name The Name to map.
     * @param string $DestName The Template to map to.
     *
     * @note If Name has already been mapped, it is silently overwritten.
     * @note If a template Name is mapped to NULL, the template is lost.
     * @note If Name is mapped to NULL, rendering will be a no-op.
     * @note If DestName doesn't exist, an empty template is created.
     */
    public function ReMap( $Name,$DestName )
    {
        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugReMap($Name,$DestName);

        if( empty($DestName) )
        {
            $this->Templates[$Name] = NULL;
        }
        else
        {
            if( isset($this->Templates[$DestName]) )
                $this->Templates[$Name] = &$this->Templates[$DestName];
            else
                $this->Templates[$Name] = Template::Init($DestName,"unknown DestName '{$DestName}'",'','   ');
        }
    }

    /**
     * Push variables into templates.
     *
     * This make arrays and objects available within a rendering template's scope.  Each
     * connected variable is available within every template, unless it gets overwritten.
     *
     * Other variables could be connected but it's not recommended.
     *
     * @param array $Vars Key/value pair of names and objects or other arrays.
     * @throws Exception Vars not an array for '$Name'
     *
     * @note Arrays are not passed by reference - they must be Connect()'d after any changes are to be made to them.
     * @note WARNING: There is no escaping - it should be done inline in the template's Body.
     *
     * @see __call() for passing variables at render time.
     */
    public function Connect( $Vars )
    {
        if( !is_array($Vars) )
            throw new Exception("Vars not an array for '$Name'");

        foreach( $Vars as $K => $V )
            $this->Connected[$K] = $V;
    }

    /**
     * Retrieve a Connect()'d variable.
     *
     * @param string $Name The Name of the Connect()'d variable.
     * @retval mixed The variable.
     * @retval NULL Name is not connected.
     *
     * @note This does not return a reference.  Use wisely.
     */
    public function GetConnect( $Name )
    {
        return empty($this->Connected[$Name])?NULL:$this->Connected[$Name];
    }


    /**
     * Perform a ReMap() or a Stack() according to a Directive.
     *
     * If $Key is Stack a Stack will be performed in which case Value will be used as
     * Name;Stack arguments to Stack().
     *
     * @param string $Key The Name to map or Stack.
     * @param string $Value The Template to map to.
     */
    public function ApplyDirective( $Key,$Value )
    {
        if( $Key === 'Stack' )
        {

            $T = explode(';',$Value);
            $this->Stack($T[0],$T[1]);
        }
        else
        {
            $this->ReMap($Key,$Value);
        }
    }

    /**
     * Add a Template to a Stack.
     *
     * A Stack groups Templates for bundled execution, which must be
     * executed using Unstack().
     *
     * @param string $Name The Template to stack.
     * @param string $Stack The Stack to add to which is created if needed.
     * @throws Exception Template '$Name' does not exist.
     *
     * @note $Name must be an original Template name, not a ReMap()'d name.
     */
    public function Stack( $Name,$Stack )
    {
        if( empty($this->Templates[$Name]) )
            throw new Exception("Template '{$Name}' does not exist for Stack '{$Stack}'.");

        if( isset($this->Stacks[$Stack]) )
            $this->Stacks[$Stack][] = $Name;
        else
            $this->Stacks[$Stack] = array($Name);
    }

    /**
     * Render each Template of a Stack.
     *
     * The Templates are rendered in the order they were stacked.  Each
     * one is rendered normally, though passing variables or having the
     * output returned is not supported.
     *
     * If $Stack does not exist this is a no-op.
     *
     * @param string $Stack The Stack to execute.
     */
    public function Unstack( $Stack )
    {
        if( isset($_SERVER[$this->DebugToken]) )
        {
            $BT = Debug::Backtrace();
            // would be nice to determine calling template from eval()
            $BT = current(Debug::BT2Str($BT,'Unstack'));
            Log::Log("\${$this->DebugToken}::Unstack($Stack) called from {$BT}",'WARN');
        }

        if( !empty($this->Stacks[$Stack]) )
        {
            foreach( $this->Stacks[$Stack] as $T )
                $this->$T();
        }
    }

    /**
     * Internal method for debugging template rendering.
     *
     * Debug info includes available variable within the scope of rendering.
     *
     * @param string $Name Name of template.
     * @param array $Args Optional render-time arguments.
     *
     * @todo would be nice to determine calling template from eval() with the bracktrace
     */
    protected function Debug__call( $Name,$Args )
    {
        $E = Is::Arr(0,$Args)?$this->Connected+$Args[0]:$this->Connected;
        $Extracts = array();
        foreach( $E as $K => $V )
        {
            $Extracts[$K] = Is::What($V);
            if( $Extracts[$K] === 'object' )
                $Extracts[$K] = get_class($V);
        }

        $BT = Debug::Backtrace();
        $BT = current(Debug::BT2Str($BT,'__call'));

        if( isset($this->Templates[$Name]) )
        {
            $T = $this->Templates[$Name];
            $Buf = "\${$this->DebugToken}::{$Name} as {$T['Name']} called from {$BT}";
        }
        else
        {
            $Buf = "\${$this->DebugToken}::{$Name} as NULL RENDER called from {$BT}";
        }

        Log::Log($Buf,'LOG',NULL,$Extracts);
    }


    /**
     * Helper method for resolving the name of a template.
     *
     * Used for in-template debugging.
     *
     * @param string $Name Name of template.
     * @retval string The original or ReMap()'d name.
     */
    protected function LookupName( $Name )
    {
        if( isset($this->Templates[$Name]) )
            return $this->Templates[$Name]['Name'];
        else
            return 'NULL';
    }


    /**
     * Internal method for debugging template name remapping.
     *
     * @param string $Name Name of template.
     * @param string $DestName Target template name.
     *
     * @todo possible to add the name of the source Directive?
     */
    protected function DebugReMap( $Name,$DestName )
    {
        $BT = Debug::Backtrace();
        foreach( array_reverse($BT) as $K => $V )
        {
            if( $V['Function'] === 'ApplyDirective' )
            {
                $BT = 'Directive';
                break;
            }
            else if( $V['Function'] === 'ReMap' )
            {
                $BT = current(Debug::BT2Str($BT,'ReMap'));
                break;
            }
        }

        if( empty($DestName) )
        {
            $Buf = "\${$this->DebugToken}::ReMap('{$Name}') to NULL RENDER";
            Log::Log($Buf,'WARN',NULL,NULL);
        }
        else if( isset($this->Templates[$DestName]) )
        {
            $T = $this->Templates[$DestName];
            $Buf = "\${$this->DebugToken}::ReMap('{$Name}') to '{$DestName}' called from $BT";
            Log::Log($Buf,'WARN',NULL,NULL);
        }
        else
        {
            $Buf = "\${$this->DebugToken}::ReMap('{$Name}') to UNKNOWN";
            Log::Log($Buf,'ERROR',NULL,NULL);
        }
    }
}

