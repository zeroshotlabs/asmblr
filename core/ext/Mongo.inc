<?php
/**
 * @file Mongo.inc Mongo connectivity.
 * @author Stackware, LLC
 * @version 4.0
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * This extension is composed of two classes:
 *  - The Mongo class which extends PHP's MongoClient class.  It includes database
 *    aliasing, returns the extension's MongoDB upon request, for a database
 *    and otherwise leaves Mongo's native functionality untouched.
 *
 *  - The MongoDB class which extends PHP's MongoDB class and implements CRUDC.
 *    It provides debugging of CRUDC operations and error handling, and otherwise
 *    leaves MongoDB's native functionality untouched.
 */
class Mongo extends \MongoClient implements Debuggable
{
    use Debugged;

    /**
     * An array of databases that are available.
     */
    protected $Aliases = array();

    /**
     * An array of GridGS databases that are available.
     */
    protected $GridFSAliases = array();


    /**
     * Return a MongoDB object by it's aliased name.
     *
     * @param string $DBAlias The aliased name of a database.
     * @throws Exception Unknown Mongo database alias '$DBAlias'.
     * @retval MongoDB A database object or native MongoGridFS object.
     *
     * @note This returns a new object each time.  If you App::Wire() the
     *       object, you must continue using the App::Wire()'d object and not
     *       request a new object from here.
     */
    public function __get( $DBAlias )
    {
        if( isset($this->Aliases[$DBAlias]) === TRUE )
        {
            if( isset($_SERVER[$this->DebugToken]) )
            {
                $BT = Debug::Backtrace();
                $BT = current(Debug::BT2Str($BT));

                Log::Log("{$DBAlias} as database {$this->Aliases[$DBAlias]} at {$BT}");
            }

            return new \asm\MongoDB($this,$this->Aliases[$DBAlias]);
        }
        else if( isset($this->GridFSAliases[$DBAlias]) === TRUE )
        {
            return parent::__get($this->GridFSAliases[$DBAlias][0])->getGridFS($this->GridFSAliases[$DBAlias][1]);
        }
        else
            throw new Exception("Unknown Mongo database alias '$DBAlias'.");
    }

    /**
     * Make a Mongo database accessible by an aliased name.
     *
     * A Mongo database cannot be accessed without first being
     * Alias()'d, even if it's to it's own name.
     *
     * Existing aliases are silently overwritten.
     *
     * @param string $DBName The name of the database in Mongo.
     * @param string $DBAlias The aliased name of the database.
     * @retval MongoDB Return the aliased database object.
     *
     * @note While this method may seem superflous, it's by design to
     *       prevent databases from being created because of typos.
     */
    public function Alias( $DBName,$DBAlias )
    {
        $this->Aliases[$DBAlias] = $DBName;
        return $this->__get($DBAlias);
    }

    /**
     * Make a Mongo GridFS database's accessible by an aliased name.
     *
     * A Mongo GridFS database cannot be accessed without first being
     * AliasGridFS()'d, even if it's to it's own name.
     *
     * @param string $DBAlias The alias of a Mongo database - must have already been Alias()'d.
     * @param string $Prefix The prefix that the GridFS collections will use.
     * @param string $GridFSAlias The aliased name of the GridFS database.
     * @param NULL $GridFSAlias The alias will be $Prefix.
     * @throws Exception Can't AliasGridFS() unknown Mongo database alias '$DBAlias'.
     *
     * @todo Review and confirm correctness.
     */
    public function AliasGridFS( $DBAlias,$Prefix,$GridFSAlias = NULL )
    {
        if( isset($this->Aliases[$DBAlias]) === TRUE )
            $this->GridFSAliases[$GridFSAlias===NULL?$Prefix:$GridFSAlias] = array($this->Aliases[$DBAlias],$Prefix);
        else
            throw new Exception("Can't AliasGridFS() unknown Mongo database alias '$DBAlias'.");
    }
}


/**
 * A MongoDB database class that implements Wireable and CRUDC.  It provides
 * debugging of CRUDC operations and error handling, and
 * otherwise leaves MongoDB's native functionality untouched.
 */
class MongoDB extends \MongoDB implements Debuggable,CRUDC
{
    use Debugged;


    /**
     * Map valid fields to an array of data.
     *
     * Map() reads the valid fields of a ValidationReport and matches them to
     * the elements of an array.  If a field name doesn't exist in the array,
     * it is skipped.
     *
     * An associative array of field name/element name overrides can also be
     * provided to rename fields in the returned array.  If the field name
     * is mapped to NULL, or to a field that doesn't exist, it is skipped.
     *
     * @param ValidationReport $VR The ValidationReport to map.
     * @param array $Src Associative array of data to map.
     * @param array $Override Array of renamed or skipped field names.
     * @retval array An associative array of field names and their values.
     *
     * @note The structure of $Override (field name=>new name) is different from
     *       that of MySQL::Map() (column name=>element name).
     * @note While the overall ValidationReport isn't checked to be valid, it
     *       likely should be before calling this method.
     * @note A valid field is one that's TRUE.  This may include an empty field
     *       which is not required.  All others are skipped, regardless of $Override.
     * @note Use a custom Struct for guaranteed consistency and complex nested
     *       array structures.
     */
    public function Map( \asm\ValidationReport $VR,$Src,$Override = array() )
    {
        $Dest = array();
        foreach( $VR as $K => $V )
        {
            if( $V !== TRUE )
                continue;

            // we have an override (either NULL or a renamed field name)
            if( array_key_exists($K,$Override) )
                $K2 = $Override[$K];
            // no override - field name is not renamed
            else
                $K2 = $K;

            // NULL override, skip element
            if( empty($K2) )
                continue;
            // field maps to element, map data, possibly with a renamed field name
            else if( array_key_exists($K,$Src) )
                $Dest[$K2] = $Src[$K];
            // field doesn't map to data, skip
            else
                continue;
        }

        return $Dest;
    }


    /*** Implement CRUDC ***/

    /**
     * Insert a record into a collection.
     *
     * @param string $Table The collection to insert into.
     * @param array $Values Key/value array to insert.
     * @throws Exception Values must be array with collection '{$Table}'.
     * @retval mixed The _id key that was inserted.
     *
     * @note This uses MongoCollection::insert() with safe=TRUE.
     */
    public function CREATE( $Table,$Values )
    {
        if( is_array($Values) === FALSE )
            throw new Exception("Values must be array with collection '{$Table}'.");

        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugCRUDC('CREATE',$Table);

        $this->{$Table}->insert($Values);

        return $Values['_id'];
    }

    /**
     * Read records from a collection.
     *
     * @param string $Table The collection to read from.
     * @param array $Constraint Key/value constraint array.
     * @param array $Columns Not implemented.
     * @param array $OrderBy Field names/directions (1 = ASC, -1 = DESC).  Valid
     *                       MongoCollection::sort() syntax is required.
     * @retval array An array of records, or an empty array.
     *
     * @note An empty constraint is allowed - all records can be read.
     */
    public function READ( $Table,$Constraint = NULL,$Columns = NULL,$OrderBy = array() )
    {
        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugCRUDC('READ',$Table);

        return iterator_to_array($this->{$Table}->find((array)$Constraint)->sort($OrderBy));
    }

    /**
     * Update a single record in a collection.
     *
     * @param string $Table The collection to update.
     * @param array $Values Key/value array of new values.
     * @param array $Constraint Key/value constraint array.
     * @retval integer The number of records updated.
     *
     * @note This will at most update only one record.
     * @note Depending on $Values, i.e. it doesn't use array($set=>array('Key'=>'Value')),
     *       the entire record could be replaced.  Read the Mongo documentation.
     * @note This uses MongoCollection::update() with safe=TRUE and multiple=FALSE.
     */
    public function UPDATE( $Table,$Values,$Constraint )
    {
        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugCRUDC('UPDATE',$Table);

        return $this->{$Table}->update($Constraint,$Values,array('multiple'=>FALSE))['n'];
    }

    /**
     * Delete a single record from a collection.
     *
     * @param string $Table The collection to delete from.
     * @param array $Constraint Key/value constraint array.
     * @retval integer The number of records deleted, which will be 0 or 1.
     *
     * @note This will at most delete only one record.
     * @note This uses MongoCollection::remove() with safe=TRUE and justOne=TRUE
     */
    public function DELETE( $Table,$Constraint )
    {
        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugCRUDC('DELETE',$Table);

        return $this->{$Table}->remove($Constraint,array('justOne'=>TRUE))['n'];
    }

    /**
     * Counts records in a collection.
     *
     * @param string $Table The collection to count records in.
     * @param array $Constraint Key/value constraint array.
     * @retval integer The count of records.
     *
     * @note An empty constraint is allowed - all records can be counted.
     */
    public function COUNT( $Table,$Constraint = NULL )
    {
        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugCRUDC('COUNT',$Table);

        return $this->{$Table}->count($Constraint);
    }

    /**
     * Helper for displaying debug information about the last CRUDC query.
     *
     * @param string $Type Type of CRUDC operation.
     * @param string $Collection Collection being executed against.
     */
    protected function DebugCRUDC( $Type,$Collection )
    {
        $BT = Debug::Backtrace();
        $BT = current(Debug::BT2Str($BT));

        Log::Log("\${$this->DebugToken}::{$Type}('{$Collection}') at {$BT}",'LOG',NULL);
    }

    /**
     * Check whether a MongoException is caused by a duplicate key conflict.
     *
     * @param Exception $E A MongoException or child class.
     * @param NULL|string $Key Optionally test whether the duplicate is on a specific key.
     * @retval boolean TRUE if the exception is due to a duplicate key conflict, and optionally of the specified key.
     * @retval NULL The exception is a duplicate, but not of the specified key.
     *
     * @todo Why does getCode() always return 0?  So we use getMessage().
     */
    public static function IsDupeE( \Exception $E,$Key = NULL )
    {
        if( is_a($E,'MongoException') && strpos($E->getMessage(),'E11000') !== FALSE )
        {
            if( $Key === NULL )
                return TRUE;
            else
                return strpos($E->getMessage(),$Key)!==FALSE?TRUE:NULL;
        }
        else
            return FALSE;
    }

    /**
     * Print the contents of a MongoCursor.
     *
     * @param MongoCursor $M The cursor to display.
     */
    public static function DisplayCursor( \MongoCursor $M )
    {
        foreach( $M as $K => $V )
        {
            echo "\r\n\r\nKEY: $K\r\n";
            var_dump($V);
        }
    }
}



/** Needs clean-up and testing/finalization/etc
class SessionStoreMongoDB implements \SessionHandlerInterface
{
    protected $MDC;

    public function __construct( \fw\MongoDB $MDB,$Collection = 'Session' )
    {
        $this->MDC = $MDB->$Collection;
    }
    public function open( $save_path,$session_id )
    {
        return TRUE;
    }
    public function close()
    {
        return TRUE;
    }
    public function read( $session_id )
    {
        $R = $this->MDC->findOne(array('_id'=>$session_id));

        if( !empty($R) )
            return $R['d'];
        else
            return '';
    }
    public function write( $session_id,$session_data )
    {
        $this->MDC->update(array('_id'=>$session_id),array('_id'=>$session_id,'cts'=>new \MongoDate,'d'=>$session_data),array('upsert'=>TRUE));
        return TRUE;
    }
    public function destroy( $session_id )
    {
        // should we not remove it and let GC handle it?
        $this->MDC->remove(array('_id'=>$session_id));
        return TRUE;
    }
    // maybe not correct - can we avoid having cts?  if not, need an index on it
    public function gc( $maxlifetime )
    {
        $this->MDC->remove(array('$lt'=>array('cts'=>new \MongoDate(time()-$maxlifetime))));
        return TRUE;
    }
}
*/
