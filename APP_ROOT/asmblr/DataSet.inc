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
class DataSet extends \fw\WireableBase implements \Iterator,\Countable,\ArrayAccess
{
    protected $MC;
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
        $this->ASC = $ASC;

        $this->Refresh();
    }

    public function Create( $Data,$O = NULL )
    {
        $D = Data::Init($this->Site_id,$this->Tag,$O===NULL?$this->NextO():Data::NormO($O),$Data);
        $this->MDC->insert($D);
        $this->Refresh();
    }

    public function Read( \MongoId $D )
    {
        return $this->MDC->findOne(array('_id'=>$D));
    }

    public function Delete( $D )
    {
        return $this->MDC->remove(array('_id'=>$D['_id'],'CurrentTS'=>$D['CurrentTS']))['n'];
    }

    public function Refresh( $ASC = NULL )
    {
        $this->ASC = is_bool($ASC)?$ASC:$this->ASC;

        $this->MC = $this->MDC->find(array('Site_id'=>$this->Site_id,'Tags.T'=>$this->Tag))->
                                sort(array('Tags.O'=>empty($this->ASC)?-1:1));

        return $this->MC;
    }

    // Add a Tag to an existing data record at an optional $O
    public function Tag( \MongoId $D,$O = NULL )
    {
        if( $O !== NULL )
            $O = Data::NormO($O);

        if( is_int($O) )
            $this->Expand($O);
        else if( $O === NULL )
            $O = $this->NextO();

        // should we check that $D belongs to our Site_id?
        $this->MDC->update(array('_id'=>$D,'Tags.T'=>array('$ne'=>$this->Tag)),
                           array('$push'=>array('Tags'=>array('T'=>$this->Tag,'O'=>$O))));

        $this->Refresh();
    }

    // remove a Tag from an existing data record
    // won't actually delete the record even if it's the last tag (will be untagged orphaned data)
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

    // returns either 0 or max + 1
    protected function NextO()
    {
        $O = $this->MDC->find(array('Site_id'=>$this->Site_id,'Tags.T'=>$this->Tag),
                              array('Tags.$'=>TRUE))->sort(array('Tags.O'=>-1))->limit(1)->getNext();

        if( empty($O) )
            return 0;
        else
            return ($O['Tags'][0]['O']+1);
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
    {
        return $this->MDC->update(array('Site_id'=>$this->Site_id,
                                           'Tags'=>array('$elemMatch'=>array('T'=>$this->Tag,'O'=>array('$gt'=>$O)))),
                                  array('$inc'=>array('Tags.$.O'=>1)),
                                  array('multiple'=>TRUE));
    }

    protected function Contract( $O )
    {
        return $this->MDC->update(array('Site_id'=>$this->Site_id,
                                           'Tags'=>array('$elemMatch'=>array('T'=>$this->Tag,'O'=>array('$gt'=>$O)))),
                                  array('$inc'=>array('Tags.$.O'=>-1)),
                                  array('multiple'=>TRUE));
    }

    // would be great to use the cursor for these?  or just create the cursor when iteration starts?
    public function offsetGet( $O )
    {
        // does this need to be elemMatch?
        $R = $this->MDC->findOne(array('Site_id'=>$this->Site_id,'Tags.T'=>$this->Tag,
                                       'Tags.O'=>Data::NormO($Key)));

        if( empty($R) )
            return NULL;
        else
            return $R['Data'];
    }

    public function offsetExists( $O )
    {
        return $this->MDC->findOne(array('Site_id'=>$this->Site_id,'Tags.T'=>$this->Tag,
                                         'Tags.O'=>Data::NormO($Key)),array('CurrentTS'=>1))===NULL?FALSE:TRUE;
    }

    public function offsetUnset( $Key )
    {
        throw new \fw\Exception("Cannot unset at $O - use Delete().");
    }

    public function count()
    {
        return $this->MC->count(TRUE);
    }

    public function rewind()
    {
        $this->MC->rewind();
    }

    public function current()
    {
        return $this->MC->current()['Data'];
    }

    public function key()
    {
        return $this->MC->key();
    }

    public function next()
    {
        return $this->MC->next()['Data'];
    }

    public function valid()
    {
        return $this->MC->valid();
    }

    public function ApplyDirective( $Key,$Value )
    {
        throw new \fw\Exception("Not supported in DataSet {$this->Collection}::{$this->Tag}");
    }
}

