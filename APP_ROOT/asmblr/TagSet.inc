<?php
namespace asm;


// Collection: TagSet
// Unique: Site / Tags.T / Tags.O
abstract class Data extends MongoStruct
{
    protected static $Skel = array('Site_id'=>'','Tags'=>array(),'CurrentTS'=>0,
                                   'Data'=>array());

    public static function Init( \MongoId $S,$Tag,$Ordering,$Data )
    {
        $Data = static::$Skel;
        $Data['Site_id'] = $S;
        $Data['Tags'] = array('T'=>$Tag,'O'=>ctype_digit((string)$Ordering)?((int)$Ordering):((string)$Ordering));
        $Data['Data'] = $Data;
        return $Data;
    }
}



// ensureIndex(array('Site'=>1,'Tags'=>1),array('unique'=>TRUE));
// Key is the position
// this loads the entire data set in upon instantiation
class DataSet extends \fw\KeyValueSet
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

        $this->Refresh();
    }

    public function Refresh()
    {
        $this->KV = $this->MDC->find(array('Site'=>$this->Site_id,'Tags.T'=>$this->Tag))->sort(array('Tags.O'=>1));
    }

    public function offsetGet( $Key )
    {
        $R = $this->MDC->findOne(array('Site'=>$this->Site_id,'Tags.T'=>$this->Tag,
                                       'Tags.O'=>ctype_digit((string)$Key)?((int)$Key):((string)$Key)));

        if( empty($R) )
            return array();
        else
            return $R;
    }

    // < prepend to beginning of set
    // > | NULL  append to end of set
    // int insert at position, pushing rest down
    // str insert with ordering but don't increment
    // because we're lazy, we insert and then do a full refresh
    public function offsetSet( $Key,$Value )
    {
        if( $Key === '<' )
        {
            $Data = Data::Init($this->Site_id,$this->Tag,0,$Value);
            $this->Expand(0);
            $this->MDC->insert($Data);
        }
        else if( $Key === '>' || $Key === NULL )
        {
            $O = $this->MDC->find(array('Tags.O'=>TRUE),
                                  array('Site'=>$this->Site_id,'Tags.T'=>$this->Tag))->sort(array('Tags.O'=>-1))->limit(1)->getNext();
            $Data = Data::Init($this->Site_id,$this->Tag,$O['Tags.O'],$Value);
            $this->MDC->insert($Data);
        }
        else if( ctype_digit((string)$Key) === TRUE )
        {
            $Data = Data::Init($this->Site_id,$this->Tag,$Key,$Value);
            $this->Expand($Key);
            $this->MDC->insert($Data);
        }
        else
        {
            $Data = Data::Init($this->Site_id,$this->Tag,$Key,$Value);
            $this->MDC->insert($Data);
        }

        $this->Refresh();
    }
    public function offsetExists( $Key )
    {
        return isset($this->KV[$Key]);
    }
    public function offsetUnset( $Key )
    {
        $O = ctype_digit((string)$Key)?((int)$Key):((string)$Key);
        $this->MDC->remove(array('Site'=>$this->Site_id,'Tags.T'=>$this->Tag,'Tags.O'=>$O));
        if( is_int($O) )
            $this->Contract($O);

        $this->Refresh();
    }

    protected function Expand( $O )
    {
        return $this->MDC->update(array('Site'=>$this->Site_id,'Tags.T'=>$this->Tag,array('Tags.O'=>array('$gte'=>(int)$O))),
                                  array('$inc'=>array('Tags.O'=>1)),
                                  array('multiple'=>TRUE));
    }
    protected function Contract( $O )
    {
        return $this->MDC->update(array('Site'=>$this->Site_id,'Tags.T'=>$this->Tag,array('Tags.O'=>array('$gte'=>(int)$O))),
                                  array('$inc'=>array('Tags.O'=>-1)),
                                  array('multiple'=>TRUE));
    }

    public function ApplyDirective( $Key,$Value )
    {
        throw new \fw\Exception("Not supported in {$this->Collection}:{$this->Tag}");
    }
    public function Import( &$ArrayRef = NULL )
    {
        throw new \fw\Exception("Not supported in {$this->Collection}:{$this->Tag}");
    }
    public function Export()
    {
        throw new \fw\Exception("Not supported in {$this->Collection}:{$this->Tag}");
    }
    public function __get( $Key )
    {
        throw new \fw\Exception("Use array access instead to __get() in {$this->Collection}:{$this->Tag}");
    }
    public function __set( $Key,$Value )
    {
        throw new \fw\Exception("Use array access instead to __set() in {$this->Collection}:{$this->Tag}");
    }
    public function __isset( $Key )
    {
        throw new \fw\Exception("Use array access instead to __isset() in {$this->Collection}:{$this->Tag}");
    }
    public function __unset( $Key )
    {
        throw new \fw\Exception("Use array access instead to __unset() in {$this->Collection}:{$this->Tag}");
    }
}

