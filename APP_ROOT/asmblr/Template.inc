<?php
namespace asm;


// Collection: TemplateSet
// Unique: DomainName
abstract class Template extends MongoStruct
{
    protected static $Skel = array('Site_id'=>'','Name'=>'','CurrentTS'=>0,
                                   'Routine'=>array(),'Body'=>'');

    // TODO: we probably want to encode/newline normalize/etc all input through out, esp. templates/etc. and content
    public static function Init( \MongoId $S,$Name,$Body = '' )
    {
        $Template = static::$Skel;
        $Template['Site_id'] = $S;
        $Template['Name'] = $Name;
        $Template['Body'] = $Body;
        return $Template;
    }

    // Routine is assumed a string and set as a struct
    public static function SetRoutine( $Routine,&$T )
    {
        if( trim($Routine) === '' )
            $T['Routine'] = array();
        else
            $T['Routine'] = \fw\Routine::Init($Routine);
    }

    // ditto per Page - Name is not saved
    public static function ToPHP( $T )
    {
        $Buf = "<?php\n\n";

        // review compatibility with Pointer routines
        $Buf .= "/*** START ROUTINE ***/";
        $Buf .= "\n\$Routine = <<<'_EO_ROUTINE_'\n".\fw\Struct::Get(0,$T['Routine'])."\n\n_EO_ROUTINE_;\n\n";

        $Buf .= "\n\n/*** START BODY ***/";
        $Buf .= "\n\$Body = <<<'_EO_BODY_'\n{$T['Body']}\n_EO_BODY_;\n\n";

        return $Buf;
    }

    // see also Page::FromPHP / Site::FromPHP
    public static function FromPHP( $PHP,&$ToTemplate )
    {
        eval('?>'.$PHP);

        $ToTemplate = static::Init($ToTemplate['Site_id'],$ToTemplate['Name'],$Body);

        static::SetRoutine($Routine,$ToTemplate);
    }
}



// hardwired to enUS but good enough for now
// also we won't support @@@ frags - what about stacking?
// note the big difference from FW standard templates - this is not Content
class TemplateSet extends MongoSet
{
    use \fw\TemplateTraits;

    protected $Site_id;


    public function __construct( \fw\MongoDB $MDB,\MongoId $Site_id = NULL,$Collection = NULL )
    {
        parent::__construct($MDB,$Collection);

        if( !empty($Site_id) )
        {
            $this->Load($Site_id);
            $this->Site_id = $Site_id;
        }
    }

    public function __call( $Name,$Args )
    {
        if( isset($_SERVER[$this->DebugToken]) )
        {
            $E = \fw\Is::Arr(0,$Args)?$this->Connected+$Args[0]:$this->Connected;
            $this->Extracts = array();
            foreach( $E as $K => $V )
            {
                $this->Extracts[$K] = \fw\Is::What($V);
                if( $this->Extracts[$K] === 'object' )
                    $this->Extracts[$K] = get_class($V);
            }

            $BT = \fw\Debug::Backtrace();
            // would be nice to determine calling template from eval()
            $BT = current(\fw\Debug::BT2Str($BT,'__call'));

            if( isset($this->Templates[$Name]) )
            {
                $T = $this->Templates[$Name];
                $Buf = "\${$this->WiredAs}::{$Name} as {$T['Name']} called from {$BT}";
                // make this noticeable if we're called from outside a template or stack
                if( strpos($BT,'eval') === FALSE && strpos($BT,'Template.inc') === FALSE )
                    $L = 'WARN';
                else
                    $L = 'LOG';
            }
            else
            {
                $Buf = "\${$this->WiredAs}::{$Name} as NULL RENDER called from {$BT}";
                $L = 'LOG';
            }

            \fw\Log::Log($Buf,$L,NULL,$this->Extracts);
        }

        if( isset($this->Templates[$Name]) )
        {
            $RenderingTemplate = $this->Templates[$Name];

            if( empty($RenderingTemplate['Routine']) )
            {
                extract(\fw\Is::Arr(0,$Args)?$this->Connected+$Args[0]:$this->Connected);

                if( $Name === 'Base' )
                {
                    eval("?>{$RenderingTemplate['Body']}");
                }
                else
                {
//                    echo "<div contenteditable=\"true\" class=\"asm-live-edit\" id=\"{$Name}\">";
                    eval("?>{$RenderingTemplate['Body']}");
//                    echo "</div>";
                }
            }
            else
            {
                if( $RenderingTemplate['Routine']['Type'] === 'Pointer' )
                    $AllowRender = $RenderingTemplate['Routine'][0]::$RenderingTemplate['Routine'][1]();
                else
                    $AllowRender = eval($RenderingTemplate['Routine'][0]);

                if( $AllowRender !== FALSE )
                {
                    extract(\fw\Is::Arr(0,$Args)===TRUE?$this->Connected+$Args[0]:$this->Connected);
                    if( $Name === 'Base' )
                    {
                        eval("?>{$RenderingTemplate['Body']}");
                    }
                    else
                    {
//                        echo "<div class=\"asm-live-edit\" id=\"{$Name}\">";
                        eval("?>{$RenderingTemplate['Body']}");
//                        echo "</div>";
                    }
                }
            }

            if( $Name === 'Base' )
            {
//                echo '<script src="/jslib/ckeditor/ckeditor.js"></script>';
            }
        }
    }

    // see also PageSet::Load
    public function Load( \MongoId $S )
    {
        $this->Templates = array();
        // ick - hopefully not too bad because there aren't a lot of templates per site
        foreach( $this->Listing($S) as $V )
            $this->Templates[$V['Name']] = $V;
    }

    public function Create( &$T )
    {
        $T['_id'] = new \MongoId;
        $this->MDC->insert($T);
        $this->Templates[$T['Name']] = $T;
    }

    // see also PageSet
    public function Read( $Name )
    {
        return $this->MDC->findOne(array('Site_id'=>$this->Site_id,'Name'=>$Name));
    }

    public function Read_id( \MongoId $T )
    {
        return $this->MDC->findOne(array('_id'=>$T));
    }

    public function Delete( $T )
    {
        return $this->MDC->remove(array('_id'=>$T['_id'],'CurrentTS'=>$T['CurrentTS']))['n'];
    }

    public function SetRoutine( $Routine,&$T )
    {
        Template::SetRoutine($Routine,$T);
        return $this->SetKV('Routine',$T['Routine'],'_id',$T);
    }

    public function SetBody( $Body,&$T )
    {
        return $this->SetKV('Body',$Body,'_id',$T);
    }

    public function SetName( $Name,&$T )
    {
        return $this->SetKV('Name',$Name,'_id',$T);
    }

    public function Listing( \MongoId $S )
    {
        // $Fields = array('Path'=>1,'Domain'=>1,'Name'=>1,'Status'=>1,'CreateTS'=>1,'UpdateTS'=>1);
        return iterator_to_array($this->MDC->find((array('Site_id'=>$S))));
    }
}

class enUSHTMLSet extends TemplateSet
{
    use \fw\enUSHTMLTraits;
}


/**
 * The following may be useful to review for adding functionality/utility and providing compatibility
 * to WP plug-ins:
 *  - http://codex.wordpress.org/Function_Reference/add_shortcode

 Possible basics for live edit.
class AsmblrFragmentSet extends \fw\HTMLSet
{
    public function __call( $Token,$Args )
    {
        ob_start();
        parent::__call($Token,$Args);
        $T = ob_get_clean();
        parent::__call('AsmblrFragmentWrap',array(array('rFrag'=>$T)));
    }
}

 */


