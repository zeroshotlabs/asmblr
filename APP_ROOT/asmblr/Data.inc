<?php
namespace asm;


// Collection: DataSet
// Unique: Site_id / Tags.T / Tags.O
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



// O is the position of the data within the set
// this loads/refreshes the entire data set as MongoCursor
// also does a lot of redundant queries that could be optimized like Refresh()

// this index may be all that we need - though it can't be unique because we can't do ordered update for Expand()
// $asmdb->DataSet->ensureIndex(array('Site_id'=>TRUE,'Tags.T'=>TRUE,'Tags.O'=>TRUE),array('unique'=>FALSE));
class DataSet extends \fw\WireableBase implements \Iterator,\Countable,\ArrayAccess
{
    protected $MC;
    protected $MDB;
    protected $MDC;
    protected $Site_id;
    protected $Tag;


    public function __construct( \fw\MongoDB $MDB,\MongoId $Site_id,$Tag,$ASC = TRUE,$Collection = NULL )
    {
        if( empty($Collection) )
            $Collection = str_replace('asm\\','',get_class($this));

        $this->MDB = $MDB;
        $this->MDC = $MDB->$Collection;
        $this->Site_id = $Site_id;
        $this->Tag = $Tag;
        $this->ASC = $ASC;

        $this->Refresh();
    }

    // change the tag of the current DataSet object - doesn't touch the database itself
    // all of these refreshes are probably overkill
    public function ReTag( $Tag )
    {
        $this->Tag = $Tag;
        $this->Refresh();
    }

    public function ReSite( \MongoId $Site_id )
    {
        $this->Site_id = $Site_id;
        $this->Refresh();
    }

    // this is always an append - returns Data struct, rather than changing parameters like the others
    public function Create( $Data )
    {
        $D = Data::Init($this->Site_id,$this->Tag,$this->NextO(),$Data);
        $this->MDC->insert($D);
        $this->Refresh();
        return $D;
    }

    public function Read( \MongoId $D )
    {
        return $this->MDC->findOne(array('_id'=>$D));
    }

    public function Move( $D,$NewO,$AsCopy = FALSE )
    {
        if( ($O = $this->OOf($D['_id'])) === NULL )
            return FALSE;

        // move to the end
        if( $NewO === NULL )
        {
            if( $AsCopy )
            {
                $D2 = Data::Init($this->Site_id,$this->Tag,$this->NextO(),$D['Data']);
                $this->MDC->insert($D2);
                $this->Refresh();
                return $D2;
            }
            else
            {
                $this->SetO($D['_id'],$this->NextO());
                $this->Contract($O);
                return $D;
            }
        }

        $NewO = Data::NormO($NewO);

        // differing O types not supported
        if( gettype($NewO) !== gettype($O) )
            return FALSE;

        // same O is a no-op
        if( $NewO === $O )
            return FALSE;

        // move right
        if( $NewO > $O )
        {
            if( $AsCopy )
            {
                $D2 = Data::Init($this->Site_id,$this->Tag,$NewO,$D['Data']);
                $this->Expand($NewO);
                $this->MDC->insert($D2);
                $this->Refresh();
                return $D2;
            }
            else
            {
                $this->Expand($NewO);
                $this->SetO($D['_id'],$NewO);
                $this->Contract($O);
                return $D;
            }
        }
        // moving left
        else
        {
            if( $AsCopy )
            {
                $D2 = Data::Init($this->Site_id,$this->Tag,$NewO,$D['Data']);
                $this->Expand($NewO);
                $this->MDC->insert($D2);
                $this->Refresh();
                return $D2;
            }
            else
            {
                $this->Expand($NewO);
                $this->SetO($D['_id'],$NewO);
                $this->Contract($O);
                return $D;
            }
        }
    }

    public function Delete( $D )
    {
        $R = $this->MDC->remove(array('_id'=>$D['_id'],'CurrentTS'=>$D['CurrentTS']))['n'];
        if( $R === 1 )
        {
            foreach( $D['Tags'] as $T )
            {
                if( is_int($T['O']) )
                    $this->Contract($T['O'],$T['T']);
            }
        }

        $this->Refresh();
        return $R;
    }

    public function SetData( $Data,&$D )
    {
        $R = $this->MDC->update(array('_id'=>$D['_id'],'CurrentTS'=>$D['CurrentTS']),
                                array('$set'=>array('Data'=>$Data)));

        if( empty($R['err']) && $R['n'] === 1 )
        {
            $D['Data'] = $Data;
            return TRUE;
        }
        else
            return FALSE;
    }

    public function Refresh( $ASC = NULL )
    {
        $this->ASC = is_bool($ASC)?$ASC:$this->ASC;

        $this->MC = $this->MDC->find(array('Site_id'=>$this->Site_id,'Tags.T'=>$this->Tag))->
                                sort(array('Tags.O'=>empty($this->ASC)?-1:1));

        return $this->MC;
    }

    // Add an existing data record at an optional $O
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
    // if it's the last tag, the record will be deleted
    public function Untag( \MongoId $D )
    {
        // more dupe queries - and so atomic!
        if( ($O = $this->OOf($D)) === NULL )
            return FALSE;

        $D2 = $this->Read($D);
        if( count($D2['Tags']) === 1 )
        {
            $R = $this->Delete($D2);
            if( is_int($O) )
                $this->Contract($O);
            return $R;
        }

        $this->MDC->update(array('_id'=>$D),
                           array('$pull'=>array('Tags'=>array('T'=>$this->Tag))));

        if( is_int($O) )
            $this->Contract($O);

        $this->Refresh();
    }

    // given an O return the full Data record
    public function Match( $O )
    {
        $O = Data::NormO($O);

        if( ($D = $this->MDC->findOne(array('Site_id'=>$this->Site_id,'Tags.T'=>$this->Tag,'Tags.O'=>$O))) !== NULL )
            return $D;
        else
            return NULL;
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

    public function OOf( \MongoId $D )
    {
        $O = $this->MDC->findOne(array('_id'=>$D,'Tags.T'=>$this->Tag),array('Tags.$'=>TRUE));

        if( empty($O) )
            return NULL;
        else
            return $O['Tags'][0]['O'];
    }

    protected function SetO( \MongoId $D,$NewO,$T = NULL )
    {
        $NewO = Data::NormO($NewO);

        return $this->MDC->update(array('_id'=>$D,'Tags.T'=>$T===NULL?$this->Tag:$T),
                                  array('$set'=>array('Tags.$.O'=>$NewO)),
                                  array('multiple'=>FALSE));
    }

    // temp. we're not having a unique index so that this works
    protected function Expand( $O,$T = NULL )
    {
//         $Q = array('Site_id'=>$this->Site_id,
//                                            'Tags'=>array('$elemMatch'=>array('T'=>$T===NULL?$this->Tag:$T,'O'=>array('$gte'=>$O))),
//                                         '$orderby'=>array('Site_id'=>-1)
//                                        );

//         $R = $this->MDC->find($Q);
//         llog(iterator_to_array($R));

        return $this->MDC->update(array('Site_id'=>$this->Site_id,
                                           'Tags'=>array('$elemMatch'=>array('T'=>$T===NULL?$this->Tag:$T,'O'=>array('$gte'=>$O)))),
                                  array('$inc'=>array('Tags.$.O'=>1)),
                                  array('multiple'=>TRUE));
    }

    protected function Contract( $O,$T = NULL )
    {
        return $this->MDC->update(array('Site_id'=>$this->Site_id,
                                           'Tags'=>array('$elemMatch'=>array('T'=>$T===NULL?$this->Tag:$T,'O'=>array('$gte'=>$O)))),
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

    // a NULL $O is appending such as $arr[] = 'data';
    // otherwise $O is a position, in which case the _id is looked up
    // if $O doesn't exist, nothing happens - maybe should exception like Unset to use Untag/Tag to move.
    public function offsetSet( $O,$Data )
    {
        if( $O === NULL )
        {
            $this->Create($Data);
        }
        else
        {
            $D = $this->Match($O);

            if( !empty($D) )
                $this->SetData($Data,$D);
        }
    }

    public function offsetExists( $O )
    {
        return $this->MDC->findOne(array('Site_id'=>$this->Site_id,'Tags.T'=>$this->Tag,
                                         'Tags.O'=>Data::NormO($Key)),array('CurrentTS'=>1))===NULL?FALSE:TRUE;
    }

    public function offsetUnset( $O )
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

