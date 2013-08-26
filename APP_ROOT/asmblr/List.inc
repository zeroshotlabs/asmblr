<?php
namespace asm;

// this will likely replace DataSet eventually
// a ListSet manages multiple ordered lists in a single collection. ListSet is
// a list of lists.  An Item is associated with a list using a Tag and the same
// item can be associated with different lists -
// ListSet also uses Item to keep an internal list of it's lists, where each
// can have an Item.List

// Collection: ListSet
// Unique: Site_id / Tags.T / Tags.O
// Type is an arbitrary scalar indexed field (with Site_id) for various queries
// Its not used for ordering or maintaining the lists

// for pulling an ordered list can't be unique because we can't do ordered update for Expand() easily
// $this->MDC->ensureIndex(array('Site_id'=>TRUE,'Tags.T'=>TRUE,'Tags.O'=>TRUE),array('unique'=>FALSE));
// for pulling the list of lists (Dictionary)
// $this->MDC->ensureIndex(array('Site_id'=>TRUE,'Type'=>TRUE,'Tags.T'=>TRUE,'Tags.O'=>TRUE),array('unique'=>FALSE));
abstract class Item extends MongoStruct
{
    protected static $Skel = array('Site_id'=>'','Type'=>'','Tags'=>array(),'CurrentTS'=>0,
                                   'Data'=>array());

    // Site_id is set by the ListSet this Item is used in
    // and so is $Tag,$Ordering
    public static function Init( $Data,$Type = '' )
    {
        $I = static::$Skel;
        $I['Type'] = $Type;
//        $D['Site_id'] = $S;
//        $D['Tags'][] = array('T'=>$Tag,'O'=>static::NormO($Ordering));
        $I['Data'] = $Data;
        return $I;
    }

    public static function SetSite_id( \MongoId $Site_id,&$I )
    {
        $I['Site_id'] = $Site_id;
    }

    // add a tag to an item at $O (int)
    // if Tag already exists in Item, the old position is overwritten
    public static function SetTag( $Tag,$O,&$I )
    {
        if( !is_int($O) )
            throw new Exception("\$O must be an int with Tag '$Tag'");

        $Done = FALSE;
        foreach( $I['Tags'] as &$T )
        {
            if( $T === $Tag )
            {
                $T['O'] = $O;
                $Done = TRUE;
                break;
            }
        }

        if( !$Done )
            $I['Tags'][] = array('T'=>$Tag,'O'=>$O);
    }

//     public static function NormO( $O )
//     {
//         if( is_int($O) )
//             return $O;
//         else
//             return ctype_digit($O)?((int)$O):$O;
//     }
}



// O is the position of the item within it's list
// this will typically use a single collection, defaulting to ListSet
// this currently sort of supports a single item being in multiple lists (Tag), remnants from DataSet
// we may remove that, having one item only be in one list at a time
// DataSet also sort of supported string positions so that lists could be sorted, for example, alphabetically
// this may too go away
class ListSet extends \fw\WireableBase
// implements \Iterator,\Countable
// ,\ArrayAccess
{
    // internal list for dictionary of other lists
    protected $DictTag;
    protected $Dictionary;

    protected $Site_id;
    protected $MDB;
    protected $MDC;
    protected $Cursor;


    public function __construct( \fw\MongoDB $MDB,\MongoId $Site_id,$Collection = NULL,$ASC = TRUE )
    {
        if( empty($Collection) )
            $Collection = str_replace('asm\\','',get_class($this));

        $this->MDB = $MDB;
        $this->MDC = $MDB->$Collection;

        $this->Site_id = $Site_id;

        $this->DictTag = "{$Collection}_".(string)$Site_id;

        $this->ASC = $ASC;
    }

    // returns mongocursor - full items are returned for now - array('Data.Tag')
    public function Lists( $Type = '' )
    {
        if( empty($Type) )
            $C = array('Site_id'=>$this->Site_id,'Tags.T'=>$this->DictTag);
        else
            $C = array('Site_id'=>$this->Site_id,'Type'=>$Type,'Tags.T'=>$this->DictTag);

        // ->sort(array('Tags.O'=>empty($this->ASC)?-1:1))
        return $this->MDC->find($C);
    }

    // for now a list doesn't have a title - just it's Tag which is stored in Data (confusing)
    // implicit list creation isn't possible - create a list here before adding an item
    // duplicate lists are not allowed
    public function NewList( $Tag,$Type = '' )
    {
        if( $this->IsList($Tag) )
            throw new Exception("List '{$Tag}' already exists.");

        $I = Item::Init(array('Tag'=>$Tag),$Type);

        Item::SetSite_id($this->Site_id,$I);
        Item::SetTag($this->DictTag,$this->NextO($this->DictTag),$I);

        $this->MDC->insert($I);
    }

    public function IsList( $Tag )
    {
        $R = $this->MDC->findOne(array('Site_id'=>$this->Site_id,'Tags.T'=>$this->DictTag,'Data.Tag'=>$Tag));
        return empty($R)?FALSE:TRUE;
    }


    // append data to the list determined by $Tag - list must already exist
    // expects just the data array and returns the Item as it was appended
    public function Append( $Data,$Tag )
    {
        if( !$this->IsList($Tag) )
            throw new Exception("List '{$Tag}' doesn't exist - use NewList() first.");

        $I = Item::Init($Data);
        Item::SetSite_id($this->Site_id,$I);
        Item::SetTag($Tag,$this->NextO($Tag),$I);

        $this->MDC->insert($I);
        return $I;
    }

    // read an Item by _id
    public function Read_id( \MongoId $Item_id )
    {
        return $this->MDC->findOne(array('_id'=>$Item_id));
    }

    public function Delete( $I )
    {
        $R = $this->MDC->remove(array('_id'=>$I['_id'],'CurrentTS'=>$I['CurrentTS']))['n'];
        if( $R === 1 )
        {
            foreach( $I['Tags'] as $T )
            {
                if( is_int($T['O']) )
                    $this->Contract($T['O'],$T['T']);
            }
        }
    }

    public function Insert( &$I,$ListTag,\MongoId $Before_id = NULL )
    {
        // insert at the end, i.e. append
        if( empty($Before_id) )
        {
            Item::SetSite_id($this->Site_id,$I);
            Item::SetTag($ListTag,$this->NextO($ListTag),$I);
            $this->MDC->insert($I);
            return TRUE;
        }
        else
        {
            $Before = $this->Read_id($Before_id);
            if( empty($Before) )
                return FALSE;

            foreach( $Before['Tags'] as $T )
                if( $T['T'] === $ListTag )
                {
                    $O = $T['O'];
                    break;
                }

            if( !isset($O) )
                return FALSE;

            Item::SetTag($ListTag,$O,$I);
            $this->Expand($O,$ListTag);
            $this->MDC->insert($I);
            return TRUE;
        }
    }

    // set entire Data for an Item - Item must exist
    public function SetData( $Data,&$I )
    {
        $R = $this->MDC->update(array('_id'=>$I['_id'],'CurrentTS'=>$I['CurrentTS']),
                                array('$set'=>array('Data'=>$Data)));

        if( empty($R['err']) && $R['n'] === 1 )
        {
            $I['Data'] = $Data;
            return TRUE;
        }
        else
            throw new Exception($R['err']);
    }

    // set a Data.Key = Value for an Item - Item must exist though key will be added if it doesn't exist
    public function SetKV( $Key,$Value,&$I )
    {
        $R = $this->MDC->update(array('_id'=>$I['_id'],'CurrentTS'=>$I['CurrentTS']),
                                array('$set'=>array("Data.{$Key}"=>$Value)));

        if( empty($R['err']) && $R['n'] === 1 )
        {
//            \fw\Log::Sys(array('_id'=>$I['_id'],'CurrentTS'=>$I['CurrentTS']));
//            \fw\Log::Sys(array("Data.{$Key}"=>$Value));
            $I['Data'][$Key] = $Value;
            return TRUE;
        }
        else
            throw new Exception($R['err']);
    }

    public function count( $Tag )
    {
    	return $this->MDC->count(array('Site_id'=>$this->Site_id,'Tags.T'=>$Tag));
    }

    // returns array to iterate over a list - only Data is returned which is different from Lists()
    // can be expanded to have sorting args/filters/etc
    public function __invoke( $Tag )
    {
        // this behavior is handy, but needs to be streamlined somehow - it's slow....?
        $R = $this->MDC->find(array('Site_id'=>$this->Site_id,'Tags.T'=>$Tag))->
                         sort(array('Tags.O'=>1));

        $R2 = array();
        foreach( $R as $V )
        {
            // bleh
            $V['Data']['_id'] = $V['_id'];
            $R2[] = $V['Data'];
        }

        return $R2;
    }

    // alias of __invoke
    public function ls( $Tag )
    {
        return $this->__invoke($Tag);
    }

    // returns either 0 or max + 1 for the given Tag
    protected function NextO( $Tag )
    {
        $O = $this->MDC->find(array('Site_id'=>$this->Site_id,'Tags.T'=>$Tag),
                              array('Tags.$'=>TRUE))->sort(array('Tags.O'=>-1))->limit(1)->getNext();

        if( empty($O) )
            return 0;
        else
            return ($O['Tags'][0]['O']+1);
    }


    // moves an Item in front of another item, or to the end of the list
    public function Move( $I,$Next_id,$ListTag )
    {

    }


    // this supports moving only within the item's list - not across lists
    public function Move33( $I,$NewO,$ListTag,$AsCopy = FALSE )
    {
        if( ($O = $this->OOf($I,$ListTag)) === NULL )
            return FALSE;

        // move to the end
        if( $NewO === NULL )
        {
            if( $AsCopy )
            {
                Item::SetTag($ListTag,$this->NextO($ListTag),$I);
                $this->MDC->insert($I);
            }
            else
            {
                $this->SetO($I,$this->NextO($ListTag),$ListTag);
                $this->Contract($O,$ListTag);
            }

            return TRUE;
        }

        $NewO = (int) $NewO;

        // same O is a no-op
        if( $NewO === $O )
            return FALSE;

        // move right
        if( $NewO > $O )
        {
            if( $AsCopy )
            {
                $D2 = Data::Init($this->Site_id,$this->Tag,$NewO,$D['Data']);
//                Item::SetTag($ListTag,$this->
                $this->Expand($NewO,$ListTag);
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

    protected function SetO( $I,$NewO,$ListTag )
    {
        return $this->MDC->update(array('_id'=>$I['_id'],'Tags.T'=>$ListTag),
                                  array('$set'=>array('Tags.$.O'=>(int)$NewO)),
                                  array('multiple'=>FALSE));
    }

    protected function OOf( $I,$ListTag )
    {
        $O = $this->MDC->findOne(array('_id'=>$I['_id'],'Tags.T'=>$ListTag),array('Tags.$'=>TRUE));

        if( empty($O) )
            return NULL;
        else
            return $O['Tags'][0]['O'];
    }

    // temp. we're not having a unique index so that this works
    protected function Expand( $O,$ListTag )
    {
//         $Q = array('Site_id'=>$this->Site_id,
//                                            'Tags'=>array('$elemMatch'=>array('T'=>$T===NULL?$this->Tag:$T,'O'=>array('$gte'=>$O))),
//                                         '$orderby'=>array('Site_id'=>-1)
//                                        );

//         $R = $this->MDC->find($Q);
//         llog(iterator_to_array($R));

        return $this->MDC->update(array('Site_id'=>$this->Site_id,
                                           'Tags'=>array('$elemMatch'=>array('T'=>$ListTag,'O'=>array('$gte'=>$O)))),
                                  array('$inc'=>array('Tags.$.O'=>1)),
                                  array('multiple'=>TRUE));
    }

    protected function Contract( $O,$ListTag )
    {
        return $this->MDC->update(array('Site_id'=>$this->Site_id,
                                           'Tags'=>array('$elemMatch'=>array('T'=>$ListTag,'O'=>array('$gte'=>$O)))),
                                  array('$inc'=>array('Tags.$.O'=>-1)),
                                  array('multiple'=>TRUE));
    }

/*
    public function Refresh( $ASC = NULL )
    {
        $this->ASC = is_bool($ASC)?$ASC:$this->ASC;

        $this->MC = $this->MDC->find(array('Site_id'=>$this->Site_id,'Tags.T'=>$this->Tag))->
                                sort(array('Tags.O'=>empty($this->ASC)?-1:1));

        return $this->MC;
    }
*/

/*
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
*/



//     public function Read( \MongoId $D )
//     {
//         return $this->MDC->findOne(array('_id'=>$D));
//     }
/*
*/
/*
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
*/

/*
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
*/
    public function ApplyDirective( $Key,$Value )
    {
        throw new \fw\Exception("Not supported in ListSet {$this->DictTag}");
    }
}

