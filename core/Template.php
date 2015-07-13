<?php
/**
 * @file Template.php Text templating and rendering.
 * @author Stackware, LLC
 * @version 4.2
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * A Template is a block of text that can be rendered.
 *
 * A Template contains a Name, an optional function, and a body.  The Name
 * must be unique within a TemplateSet.
 *
 * If a function is present, it is executed before rendering occurs.  If the
 * function returns FALSE, rendering will not occur. A template function should
 * be prototyped as:
 *  @code funcname( $Connected,$Args,$App ) @endcode
 *
 * where @c $Connected is an array of the TemplateSet::Connect()'d variables, @c $Args are render-time
 * arguments, and @c $App is the application's App object.
 *
 * Templates are rendered and managed by a TemplateSet, together forming the "view layer".
 */
abstract class Template extends Struct
{
    /**
     * @var array $Skel
     * The base structure of a Template.
     */
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
 * This is the default templating mechanism in asmblr.
 *
 * TemplateSet is generally extended to provide locale specific functionality,
 * for example enUSHTMLSet.
 *
 * @todo Cacheable templates - Some templates, like a subnav menu, can be
 *       expensive to create and don't change much - should be savable to disk/app file,
 *       or perhaps using a cnvyrc mechanism.
 */
class TemplateSet implements Debuggable,Directable
{
    use Debugged;

    /**
     * @var boolean $DebugDisplay
     * Whether to display debug info to the browser.
     *
     * @see TemplateSet::DebugOn()
     */
    protected $DebugDisplay = FALSE;

    /**
     * @var App $App
     * The application's App object.
     */
    protected $App;

    /**
     * @var array $Templates
     * Array of Template Structs that this TemplateSet manages.
     *
     * @note Unless App::$CacheApp is TRUE, these won't contain a body (it's pulled from the filesystem upon render).
     */
    protected $Templates = array();

    /**
     * @var array $Connected
     * Array of TemplateSet::Connect()'d variables that are made
     * available within all templates during rendering.
     */
    protected $Connected = array();

    /**
     * @var array $Stacks
     * Template stacks for grouped rendering.
     *
     * @see TemplateSet::Stack
     * @see TemplateSet::Unstack
     */
    protected $Stacks = array();

    /**
     * Create a TemplateSet.
     *
     * @param App $App Application's App object.
     * @todo This is limited to use only $App->Templates meaning only a single template set per application unless extended.
     */
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
     * Removes a mapping.
     *
     * This is a shorthand for ReMap()'ing to NULL which prevents a Name from rendering anything.
     *
     * @param string $Name The Name to unset.
     */
    public function __unset( $Name )
    {
        $this->ReMap($Name,NULL);
    }

    /**
     * Retrieve the un-rendered body of a Template.
     *
     * @param string $Name The name of the template.
     * @retval string The body of the template.
     * @retval NULL The template was not found.
     */
    public function __get( $Name )
    {
        if( isset($this->Templates[$Name]) )
        {
            if( empty($this->Templates[$Name]['Path']) )
                return $this->Templates[$Name]['Body'];
            else
                return file_get_contents($this->Templates[$Name]['Path']);
        }
        else
            return NULL;
    }

    /**
     * Render and output a Template that exists in this TemplateSet.
     *
     * The rendered Template is output directly.
     *
     * Rendering begins with the named Template.  The Template Name is resolved
     * as follows:
     *  - The Name is checked for in $Templates and if found that Template is rendered.  Note
     *    that the Name may have been ReMap()'d to another template, thus causing that
     *    template to render instead.
     *  - If Name is not found, or has been mapped to NULL, nothing is rendered.
     *
     * If the rendering Template has a function defined, it will be executed prior to
     * rendering.  If the function returns FALSE, rendering for that template will not occur.
     *
     * Passing render-time arguments is possible by calling this method with an associative array.  Each
     * element of the array will be made available as a variable in the Template's body during rendering.
     *  @code
     *   $Data = array('FirstName'=>'John','LastName'=>'Doe','Items'=>array('One','Two'));
     *   $this->Header($Data);
     *  @endcode
     *
     * This will create three variables - $FirstName, $LastName and $Items - in the
     * Template body's scope during rendering.
     *
     * All rendering Templates also have access to variables that have been Connect()'d,
     * and to the TemplateSet object itself, available as @c $this.
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
     * @param array $Args Optional render-time arguments as an associative array of keys/value variables.
     *
     * @note This uses eval() to do the rendering.
     * @note Template functions cannot ReMap() the template they are executing under, though they can
     *       manually render another template and then return FALSE.
     * @note WARNING: There is no automatic escaping - it should be done explicitly in the body.
     * @note No error is raised if the Template Name isn't found and debugging is off.
     *
     * @todo We'd really like to have the render function do a ReMap() to change the template that
     *       ends up getting rendered.
     */
    public function __call( $Name,$Args )
    {
        if( isset($_SERVER[$this->DebugToken]) )
            $this->Debug__call($Name,$Args);

        if( isset($this->Templates[$Name]) )
        {
            $RenderingTemplate = $this->Templates[$Name];

            if( empty($RenderingTemplate['Function']) )
            {
                // scope the connected variables plus any arguments
                foreach( $this->Connected as $K => $V )
                    $$K = $V;

                if( !empty($Args[0]) )
                {
                    foreach( $Args[0] as $K => $V )
                        $$K = $V;
                }

                // this is clunky for now but for dev/non-caching needed for easy debugging
                // and proper error messages from PHP

                // assume we haven't loaded things from disk, or that we have and we eval()
                // same below - App::BuildFile() prepends the opening tag for eval()
                // we don't check CacheApp since for @@@ frags they will always be in body
                if( empty($RenderingTemplate['Path']) )
                    eval($RenderingTemplate['Body']);
                else
                    include $RenderingTemplate['Path'];
            }
            else
            {
                // the function should expect arguments $this->Connected,$Args,$App
                if( ($RenderingTemplate['Function']($this->Connected,$Args,$this->App)) !== FALSE )
                {
                    // scope the connected variables plus any arguments
                    // we have to do this here again since our function may connect variables... probably a slicker way...
                    foreach( $this->Connected as $K => $V )
                        $$K = $V;

                    if( !empty($Args[0]) )
                    {
                        foreach( $Args[0] as $K => $V )
                            $$K = $V;
                    }

                    if( empty($RenderingTemplate['Path']) )
                        eval($RenderingTemplate['Body']);
                    else
                        include $RenderingTemplate['Path'];
                }
            }
        }
    }

    /**
     * Load a template from a string.
     *
     * This will load the string as a template and create a Template Struct for it.  The Struct
     * may then be returned, or added to the TemplateSet's managed templates.
     *
     * @param string $Name The name of the template.
     * @param string $Str The contents of the template.
     * @param App $app The application's object.
     * @param boolean $Return TRUE to return the Template Struct, otherwise added to the TemplateSet.
     * @retval Template Template Struct if $Return is TRUE.
     *
     * @note This supports $Str as only a single template - \@\@\@ parsing isn't supported.
     * @note The $Name is used verbatim without any prefixing.
     * @note An existing template of the same name will be clobbered unless $Return is TRUE.
     * @note The template is ephemeral and not written to disk and the Path must be kept empty.
     * @note Setting a function for the template is not supported.  If the template exists and has a function, it will be preserved.
     *
     * @see TemplateSet::LoadFile()
     */
    public function LoadString( $Name,$Str,\asm\App $app,$Return = FALSE )
    {
        if( !empty($app->Manifest['Templates'][$Name]) )
            $F = $app->Manifest['Templates'][$Name];
        else
            $F = array();

        if( $Return )
            return Template::Init($Name,'',$F,"?>{$Str}");
        else
            $this->Templates[$Name] = Template::Init($Name,'',$F,"?>{$Str}");
    }


    /**
     * Load a template from a file.
     *
     * This will load a template from disk and create a Template Struct for it.  The Struct may
     * then be returned, or added to the TemplateSet's managed templates.
     *
     * @param string $Path The absolute path to the template file.
     * @param App $app The application's object.
     * @param boolean $Return TRUE to return the Template Struct, otherwise added to the TemplateSet.
     * @retval Template Template Struct if $Return is TRUE.
     *
     * @note This is typically only used for loading non-standard templates, like SQL.
     * @note Unlike \@\@\@ parsing in App, the parent's directory isn't added as a prefix to the template name.
     * @todo This needs to be reviewed/merged with App::BuildApp() template loading.
     */
    public function LoadFile( $Path,\asm\App $app,$Return = FALSE )
    {
        $P = Path::Init($Path);
        $P = Path::Bottom($P,2);

        if( $P['Segments'][0] !== 'templates' )
            $Prefix = $P['Segments'][0].'_';
        else
            $Prefix = '';

        $Buf = file_get_contents($Path);

        if( strpos(substr($Buf,0),"\n@@@") === FALSE )
        {
            $P['Segments'][1] = pathinfo($P['Segments'][1],PATHINFO_FILENAME);

            if( !empty($app->Manifest['Templates'][$Prefix.$P['Segments'][1]]) )
                $F = $app->Manifest['Templates'][$Prefix.$P['Segments'][1]];
            else
                $F = array();

            if( $Return )
                return Template::Init($Prefix.$P['Segments'][1],$K,$F,"?>{$Buf}");
            else
                $this->Templates[$Prefix.$P['Segments'][1]] = Template::Init($Prefix.$P['Segments'][1],$Path,$F,"?>{$Buf}");
        }
        else
        {
            $B = preg_split("/\s*@@@(\w+[a-zA-Z0-9\-]*)/m",$Buf,0,PREG_SPLIT_DELIM_CAPTURE|PREG_SPLIT_NO_EMPTY);
            $CNT = count($B);
            $Templates = array();
            for( $i = 0; $i < $CNT; $i+=2 )
            {
                if( !empty($app->Manifest['Templates'][$B[$i]]) )
                    $F = $app->Manifest['Templates'][$B[$i]];
                else
                    $F = array();

                $Templates[$B[$i]] = Template::Init($B[$i],'',$F,"?>{$B[$i+1]}");
            }

            if( $Return )
            {
                return $Templates;
            }
            else
            {
                foreach( $Templates as $K => $V )
                {
                    $this->Templates[$K] = $V;
                }
            }
        }
    }

    /**
     * Retrieve a Template Struct by name.
     *
     * The Struct will contain the body only if App::$CacheApp is TRUE.
     *
     * @param string $Name The name of the template.
     * @retval array The Template Struct.
     * @retval NULL Template not found.
     */
    public function Read( $Name )
    {
        if( empty($this->Templates[$Name]) )
            return NULL;
        else
            return $this->Templates[$Name];
    }

    /**
     * Render a Template, returning it's output by default.
     *
     * Helper method for __call() for rendering Templates.
     *
     * @param string $Name A ReMap()'d Name or Template Name to render.
     * @param boolean $Return FALSE to directly output the rendered template.
     * @retval string The rendered template or NULL if direct output.
     *
     * @note Passing render-time arguments is not supported.
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
     * This will cause $DestName to be rendered when $Name is called.  It's used to dynamically
     * render different templates.
     *
     * TemplateSet directives perform this operation.
     *
     * @param string $Name The Name to map.
     * @param string $DestName The Template to map to.
     *
     * @note If Name has already been mapped, it is silently overwritten.
     * @note If $DestName is empty, rendering of $Name will be a no-op and the original template is lost.
     * @note If $DestName doesn't exist, an placeholder template is created.
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
     * Add a Template to a stack.
     *
     * A Stack groups Templates for bundled execution at a later time, which must be
     * performed using Unstack().
     *
     * TemplateSet directives with a key of @c Stack perform this operation.
     *
     * @param string $Name The Template name to stack.
     * @param string $Stack The Stack to add to which is created if needed.
     * @throws Exception Template '$Name' does not exist for stack $Stack.
     *
     * @note This won't work for nested templates, such as admin_js_login.
     */
    public function Stack( $Name,$Stack )
    {
        if( empty($this->Templates[$Name]) )
            throw new Exception("Template '{$Name}' does not exist for stack '{$Stack}'.");

        if( isset($this->Stacks[$Stack]) )
            $this->Stacks[$Stack][] = $Name;
        else
            $this->Stacks[$Stack] = array($Name);
    }

    /**
     * Render the templates of a stack.
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
     * Push variables into templates.
     *
     * This make variables available within a rendering template. Each connected variable is
     * available within every template, unless it gets overwritten.
     *
     * @param array $Vars Key/value pair of variable names and their values.
     * @throws Exception Vars not an array for Connect().
     *
     * @note Arrays are not passed by reference - they must be Connect()'d after any changes are to be made to them.
     * @note WARNING: There is no automatic escaping - it should be done explicitly in the body.
     *
     * @see __call() for passing render-time variables.
     */
    public function Connect( $Vars )
    {
        if( !is_array($Vars) )
            throw new Exception('Vars not an array for Connect().');

        foreach( $Vars as $K => $V )
            $this->Connected[$K] = $V;
    }

    /**
     * Retrieve a Connect()'d variable.
     *
     * @param string $Name The Name of the Connect()'d variable to retrieve.
     * @param NULL $Name Return all Connect()'d variables.
     * @retval mixed The variable.
     * @retval NULL Name is not connected.
     *
     * @note This does not return a reference.  Use wisely.
     */
    public function GetConnect( $Name = NULL )
    {
        if( empty($Name) )
            return $this->Connected;
        else
            return empty($this->Connected[$Name])?NULL:$this->Connected[$Name];
    }

    /**
     * Perform a ReMap() or a Stack() according to a directive.
     *
     * If $Key is @c Stack a Stack() will be performed.  $Value is expected to
     * be a string of the form @code TemplateName;StackName @endcode
     *
     * All other keys will cause a ReMap() to occur.
     *
     * @param string $Key The Name to map or Stack.
     * @param string $Value The Template to ReMap() or Stack().
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
     * Enable debugging, overridding default Label behavior.
     *
     * Log messages are always labeled with the class name.
     *
     * @param boolean $Label TRUE to output debug info.
     */
    public function DebugOn( $Label = NULL )
    {
        $this->DebugToken = get_class($this);
        $_SERVER[$this->DebugToken] = TRUE;

        if( $Label === TRUE )
            $this->DebugDisplay = TRUE;
    }

    /**
     * Internal method for debugging template rendering.
     *
     * Debug info includes available variables within the scope of rendering.
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

        if( !empty($this->DebugDisplay) )
            echo "<div style=\"padding: 0; margin: 0; font-weight: bold; font-size: 10px;\">$Buf</div>";

        Log::Log($Buf,'LOG',NULL,$Extracts);
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
            if( !empty($this->DebugDisplay) )
                echo "<div style=\"padding: 0; margin: 0; color: orange; font-weight: bold; font-size: 10px;\">$Buf</div>";
            Log::Log($Buf,'WARN',NULL,NULL);
        }
        else if( isset($this->Templates[$DestName]) )
        {
            $T = $this->Templates[$DestName];
            $Buf = "\${$this->DebugToken}::ReMap('{$Name}') to '{$DestName}' called from $BT";
            if( !empty($this->DebugDisplay) )
                echo "<div style=\"padding: 0; margin: 0; color: green; font-weight: bold; font-size: 10px;\">$Buf</div>";
            Log::Log($Buf,'WARN',NULL,NULL);
        }
        else
        {
            $Buf = "\${$this->DebugToken}::ReMap('{$Name}') to UNKNOWN";
            if( !empty($this->DebugDisplay) )
                echo "<div style=\"padding: 0; margin: 0; color: red; font-weight: bold; font-size: 10px;\">$Buf</div>";
            Log::Log($Buf,'ERROR',NULL,NULL);
        }
    }

    /**
     * Helper method for resolving the name of a template.
     *
     * Used in-template for debugging purposes.
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
}

