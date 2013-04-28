<?php
namespace asm;

// ensureIndex(array('Site'=>1,'Tags'=>1),array('unique'=>TRUE));
// Key is the position
// this loads the entire tag set in upon instantiation
class TagSet extends \fw\KeyValueSet
{
    protected $MDB;
    protected $MDC;
    protected $Site_id;
    protected $Tag;


    public function __construct( \fw\MongoDB $MDB,\MongoId $Site_id,$Tag,$Collection = NULL )
    {
        $this->MDB = $MDB;
        if( empty($Collection) )
            $Collection = str_replace('asm\\','',get_class($this));

        $this->MDC = $MDB->$Collection;
        $this->Site_id = $Site_id;
        $this->Tag = $Tag;

        $this->KV = $this->MDC->find(array('Site'=>$this->Site_id,'Tags.T'=>$this->Tag))->sort(array('Tags.O'=>1));
    }

    public function offsetGet( $Key )
    {

        return $this->MDC->findOne(array('Site'=>$this->Site_id,'Tags.T'=>$this->Tag,
                                         'Tags.O'=>ctype_digit((string)$Key)?((int)$Key):((string)$Key)));
    }

    public function offsetSet( $Key,$Value )
    {
        // append
        if( $Key === NULL )

        $this->__set($Key,$Value);
    }
    public function offsetExists( $Key )
    {
        return $this->__isset($Key);
    }
    public function offsetUnset( $Key )
    {
        $this->__unset($Key);
    }

    /**
     * Implement Wireable interface, setting a key/value pair as a \c Directive.
     * @param string $Key The name of the key to set.
     * @param mixed $Value The value to set.
     * @retval void
     */
    public function ApplyDirective( $Key,$Value )
    {
        throw new Exception("Not supported in {$this->Collection}:{$this->Tag}");
    }

    public function Import( &$ArrayRef = NULL )
    {
        throw new Exception("Not supported in {$this->Collection}:{$this->Tag}");
    }
    public function Export()
    {
        throw new Exception("Not supported in {$this->Collection}:{$this->Tag}");
    }
    public function __get( $Key )
    {
        throw new Exception("Use array access instead to __get() in {$this->Collection}:{$this->Tag}");
    }
    public function __set( $Key,$Value )
    {
        throw new Exception("Use array access instead to __set() in {$this->Collection}:{$this->Tag}");
    }
    public function __isset( $Key )
    {
        throw new Exception("Use array access instead to __isset() in {$this->Collection}:{$this->Tag}");
    }
    public function __unset( $Key )
    {
        throw new Exception("Use array access instead to __unset() in {$this->Collection}:{$this->Tag}");
    }
}




// to be updated separately; update search, other dependencies?
// Execute?
class PageSet extends MongoSet
{
    use \fw\PageTraits;


    public function Create( &$P )
    {
        $P['_id'] = new \MongoId;
        $this->MDC->insert($P);
    }

    public function Read( \MongoId $P )
    {
        return $this->MDC->findOne(array('_id'=>$P));
    }

    public function Delete( $P )
    {
        return $this->MDC->remove(array('_id'=>$P['_id'],'CurrentTS'=>$P['CurrentTS']))['n'];
    }

    public function SetStatus( $Status,&$P )
    {
        return $this->SetKV('Status',$Status,'_id',$P);
    }

    public function SetName( $Name,&$P )
    {
        return $this->SetKV('Name',$Name,'_id',$P);
    }

    public function SetRoutine( $Routine,&$P )
    {
        return $this->SetKV('Routine',$Routine,'_id',$P);
    }

    public function SetDirectives( $Directives,&$P )
    {
        return $this->SetKV('Directives',$Directives,'_id',$P);
    }

    public function Listing( \MongoId $S )
    {
        // $Fields = array('Path'=>1,'Domain'=>1,'Name'=>1,'Status'=>1,'CreateTS'=>1,'UpdateTS'=>1);
        return iterator_to_array($this->MDC->find((array('Site_id'=>$S))));
    }

    // TODO: debugging
    public function Match( \MongoId $S,$Path )
    {
        $T = \fw\Path::Init(strtolower('/'.$Path));
        $DP = \fw\Path::ToURLString($T);

        if( ($P = $this->MDC->findOne(array('Site_id'=>$S,'Path'=>$DP))) !== NULL )
            return ($this->Matched[] = $P);
        else
            return NULL;
    }
}


