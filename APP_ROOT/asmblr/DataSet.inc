<?php
namespace asm;


// Collection: DataSet
// Unique: Site / Tags.T / Tags.O
abstract class Data extends MongoStruct
{
    protected static $Skel = array('Site_id'=>'','Tags'=>array(),'CurrentTS'=>0,
                                   'Data'=>array());

    public static function Init( \MongoId $S,$Tag,$Ordering,$Data )
    {
        $D = static::$Skel;
        $D['Site_id'] = $S;
        $D['Tags'][] = array('T'=>$Tag,'O'=>static::NormO($Ordering));
        $D['Data'] = $Data;
        return $D;
    }

    public static function NormO( $O )
    {
        if( is_int($O) )
            return $O;
        else
            return ctype_digit($O)?((int)$O):$O;
    }
}



// ensureIndex(array('Site_id'=>1,'Tags'=>1),array('unique'=>TRUE));
// Key is the position
// this loads the entire data set as MongoCursor - a lot of long running cursors on the server?
// also does a lot of redundant queries that could be optimized like Refresh()
class DataSet extends \fw\KeyValueSet
{
    protected $MDB;
    protected $MDC;
    protected $Site_id;
    protected $Tag;


    public function __construct( \fw\MongoDB $MDB,\MongoId $Site_id,$Tag,$ASC = TRUE,$Collection = NULL )
    {
        $this->MDB = $MDB;
        if( empty($Collection) )
            $Collection = str_replace('asm\\','',get_class($this));

        $this->MDC = $MDB->$Collection;
        $this->Site_id = $Site_id;
        $this->Tag = $Tag;

        $this->Refresh($ASC);
    }

    public function Refresh( $ASC = TRUE )
    {
        $this->KV = $this->MDC->find(array('Site_id'=>$this->Site_id,'Tags.T'=>$this->Tag))->
                                sort(array('Tags.O'=>empty($ASC)?-1:1));

        return $this->KV;
    }

    public function count()
    {
        return $this->KV->count(TRUE);
    }

    public function rewind()
    {
        $this->KV->rewind();
    }

    public function current()
    {
        return $this->KV->current()['Data'];
    }

    public function key()
    {
        return $this->KV->key();
    }

    public function next()
    {
        return $this->KV->next()['Data'];
    }

    public function valid()
    {
        return $this->KV->valid();
    }

    public function offsetGet( $Key )
    {
        $R = $this->MDC->findOne(array('Site_id'=>$this->Site_id,'Tags.T'=>$this->Tag,
                                       'Tags.O'=>Data::NormO($Key)));

        if( empty($R) )
            return array();
        else
            return $R;
    }

    // < prepend to beginning of set or just use $ds[]
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
            $Data = Data::Init($this->Site_id,$this->Tag,$this->TagMax(),$Value);
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

    // is this correct?  same as Untag?  perhaps this actually deletes whereas
    // Untag never can (though it can leave orphans)
    public function offsetUnset( $Key )
    {
        $O = Data::NormO($Key);
        $this->MDC->remove(array('Site_id'=>$this->Site_id,'Tags.T'=>$this->Tag,'Tags.O'=>$O));

        if( is_int($O) )
            $this->Contract($O);

        $this->Refresh();
    }

    public function Tag( \MongoId $D,$O = NULL )
    {
        if( $O !== NULL )
            $O = Data::NormO($O);

        if( is_int($O) )
            $this->Expand($O);
        else if( $O === NULL )
            $O = $this->TagMax()+1;

        $this->MDC->update(array('_id'=>$D,'Tags.T'=>array('$ne'=>$this->Tag)),
                           array('$push'=>array('Tags'=>array('T'=>$this->Tag,'O'=>$O))));

        $this->Refresh();
    }

    public function Untag( \MongoId $D )
    {
        if( ($O = $this->OOf($D)) === NULL )
            return FALSE;

        $this->MDC->update(array('_id'=>$D),
                           array('$pull'=>array('Tags'=>array('T'=>$this->Tag))));

        if( is_int($O) )
            $this->Contract($O);

        $this->Refresh();
    }

//     public function Move( \MongoId $D,$NewO )
//     {
//         if( ($OldO = $this->OOf($D)) === NULL )
//             return FALSE;

//         // cannot move to the same place
//         if( $OldO === $NewO )
//             return FALSE;

//         // cannot move between different O types
//         if( gettype($OldO) !== gettype($NewO) )
//             throw new Exception("Cannot Move() {$D} from {$OldO} to {$NewO}.");

//         // if a string O then we can just set it - might cause a dupe exception
//         if( is_string($OldO) )
//         {
//             $this->Set($D,$NewO);
//             return TRUE;
//         }
//         // move higher
//         else if( $NewO > $OldO )
//         {
//             $this->Expand($NewO);

//         }
//         // move lower
//         else
//         {
//         }
//     }

//     // this only sets the value - internal function - use Move()
//     protected function SetO( \MongoId $D,$NewO )
//     {
//         $this->MDC->update(array('_id'=>$D,'Tags.T'=>$this->Tag),
//                            array('$set'=>array('Tags.$.O'=>$NewO)));

//         $this->Refresh();
//     }

    protected function TagMax()
    {
        $O = $this->MDC->find(array('Site_id'=>$this->Site_id,'Tags.T'=>$this->Tag),
                              array('Tags.$'=>TRUE))->sort(array('Tags.O'=>-1))->limit(1)->getNext();

        if( empty($O) )
            return 0;
        else
            return ($O['Tags'][0]['O']);
    }

    protected function OOf( \MongoId $D )
    {
        $O = $this->MDC->findOne(array('_id'=>$D,'Tags.T'=>$this->Tag),array('Tags.$'=>TRUE));

        if( empty($O) )
            return NULL;
        else
            return $O['Tags'][0]['O'];
    }

    protected function Expand( $O )
    {var_dump($O);
        $R = $this->MDC->find(array('Site_id'=>$this->Site_id,'Tags.T'=>$this->Tag,'Tags.O'=>1));



        // array('$gte'=>(int)$O)
        var_dump(iterator_to_array($R));

        // Tags.O must be specified before Tags.T otherwise mongo updates any old tag
        return $this->MDC->update(array('Site_id'=>$this->Site_id,'Tags.O'=>array('$gte'=>(int)$O),'Tags.T'=>$this->Tag),
                                  array('$inc'=>array('Tags.$.O'=>1)),
                                  array('multiple'=>TRUE));
    }

    protected function Contract( $O )
    {
        // Tags.O must be specified before Tags.T otherwise mongo updates any old tag
        return $this->MDC->update(array('Site_id'=>$this->Site_id,'Tags.O'=>array('$gt'=>(int)$O),'Tags.T'=>$this->Tag),
                                  array('$inc'=>array('Tags.$.O'=>-1)),
                                  array('multiple'=>TRUE));
    }

    public function ApplyDirective( $Key,$Value )
    {
        throw new \fw\Exception("Not supported in DataSet {$this->Collection}::{$this->Tag}");
    }
    public function Import( &$ArrayRef = NULL )
    {
        throw new \fw\Exception("Not supported in DataSet {$this->Collection}::{$this->Tag}");
    }
    public function Export()
    {
        throw new \fw\Exception("Not supported in DataSet {$this->Collection}::{$this->Tag}");
    }
    public function __get( $Key )
    {
        throw new \fw\Exception("Use array access instead of __get() in DataSet {$this->Collection}:{$this->Tag}");
    }
    public function __set( $Key,$Value )
    {
        throw new \fw\Exception("Use array access instead of __set() in DataSet {$this->Collection}:{$this->Tag}");
    }
    public function __isset( $Key )
    {
        throw new \fw\Exception("Use array access instead of __isset() in DataSet {$this->Collection}:{$this->Tag}");
    }
    public function __unset( $Key )
    {
        throw new \fw\Exception("Use array access instead of __unset() in DataSet {$this->Collection}:{$this->Tag}");
    }
}

