<?php
namespace asm;


// Collection: TemplateSet
// Unique: DomainName
abstract class Template extends MongoStruct
{
    protected static $Skel = array('Site_id'=>'','Name'=>'','CurrentTS'=>0,
                                   'Body'=>'','Routine'=>array());

    // TODO: we probably want to encode/newline normalize/etc all input through out, esp. templates/etc. and content
    public static function Init( \MongoId $S,$Name,$Body = '' )
    {
        $Template = static::$Skel;
        $Template['Site_id'] = $S;
        $Template['Name'] = $Name;
        $Template['Body'] = $Body;
        return $Template;
    }
}



// hardwired to enUS but good enough for now
// also we won't support @@@ frags - what about stacking?
// note the big difference from FW standard templates - this is not Content
class TemplateSet extends MongoSet
{
    use \fw\TemplateTraits;


    public function Load( \MongoId $S )
    {
        $this->Templates = array();
        // ick - hopefully not too bad because there aren't a lot of templates per site
        foreach( $this->Listing($S) as $V )
            $this->Templates[$V['Name']] = $V;
    }

    public function Create( &$T )
    {
        $this->MDC->insert($T);
        $this->Templates[$T['Name']] = $T;
    }

    public function Read( $Name )
    {
        return isset($this->Templates[$Name])?$this->Templates[$Name]:NULL;
    }

    public function Delete( $T )
    {
        return $this->MDC->remove(array('_id'=>$T['_id'],'CurrentTS'=>$T['CurrentTS']))['n'];
    }

    public function SetRoutine( $Routine,&$T )
    {
        return $this->SetKV('Routine',$Routine,array('Site_id','Name'),$T);
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


