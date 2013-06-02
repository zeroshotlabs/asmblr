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

    protected $Site_id;

    // see also PageSet
    public function __construct( \fw\MongoDB $MDB,\MongoId $Site_id = NULL,$Collection = NULL )
    {
        parent::__construct($MDB,$Collection);

        if( !empty($Site_id) )
        {
            $this->Load($Site_id);
            $this->Site_id = $Site_id;
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
        return $this->SetKV('Routine',$Routine,'_id',$T);
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


