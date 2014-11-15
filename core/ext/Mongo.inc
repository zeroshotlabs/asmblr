<?php
/**
 * @file Mongo.inc Mongo connectivity.
 * @author Stackware, LLC
 * @package Mongo
 * @version 4.2
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 *
 * This extension is composed of two classes, Mongo and Mongodb, each extending PHP's
 * native classes, MongoClient and MongoDB, respectively.  They add functionality
 * to each class but do not interfere with native functionality.
 */
namespace asm;


/**
 * asmblr's Mongo class extends PHP's native MongoClient class and adds database aliasing
 * to avoid typos when performing database operations.  Database aliasing ensures that only
 * predefined database names are used.
 *
 * Native functionality is untouched.
 */
class Mongo extends \MongoClient implements Debuggable
{
    use Debugged;

    /**
     * @var array $Aliases
     * An array of database names that are available.
     */
    protected $Aliases = array();

    /**
     * @var array $GridFSAliases
     * An array of GridGS database names that are available.
     */
    protected $GridFSAliases = array();


    /**
     * Return a asm::MongoDB object by it's aliased name.
     *
     * @param string $DBAlias The aliased name of a database.
     * @throws Exception Unknown Mongo database alias '$DBAlias'.
     * @retval asm::MongoDB A database object.
     * @retval MongoGridFS Native grid FS class.
     *
     * @note This returns a new object each time.
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
     * Alias()'d, even if it's to it's own name.  This is done to prevent typos
     * when trying to perform database operations.
     *
     * Existing aliases are silently overwritten.
     *
     * @param string $DBName The name of the database in Mongo.
     * @param string $DBAlias The aliased name of the database.
     * @retval asm::MongoDB Return the aliased database object.
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
 * asmblr's MongoDB class PHP's native class of the same name and adds CRUDC operations,
 * debugging, data-mapping convenience methods, improved error handling, and other helper methods.
 *
 * Native functionality is untouched.
 */
class MongoDB extends \MongoDB implements Debuggable,CRUDC
{
    use Debugged;


    /**
     * Insert a record into a collection.
     *
     * @param string $Table The collection to insert into.
     * @param array $Values Key/value array to insert.
     * @throws Exception Values must be array with collection '{$Table}'.
     * @retval mixed The _id key that was inserted.
     *
     * @note This uses MongoCollection::insert().
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
     * @param array $Columns Column/boolean pairs to include or exclude columns.
     * @param array $OrderBy Field names/directions (1 = ASC, -1 = DESC).  Valid MongoCollection::sort() syntax is required.
     * @retval array An array of records, or an empty array.
     *
     * @note An empty constraint is allowed - all records can be read.
     * @note This uses MongoCollection::find().
     */
    public function READ( $Table,$Constraint = NULL,$Columns = NULL,$OrderBy = array() )
    {
        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugCRUDC('READ',$Table);

        return iterator_to_array($this->{$Table}->find((array)$Constraint,empty($Columns)?array():$Columns)->sort($OrderBy));
    }

    /**
     * Update a single record in a collection.
     *
     * @param string $Table The collection to update.
     * @param array $Values Key/value array of new values.
     * @param array $Constraint Key/value constraint array.
     * @retval integer 0 or 1
     *
     * @note This will at most update only one record.
     * @note Depending on $Values, i.e. it doesn't use array($set=>array('Key'=>'Value')),
     *       the entire record could be replaced.  Read the Mongo documentation.
     * @note This uses MongoCollection::update() with multiple=FALSE.
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
     * @retval integer 0 or 1.
     *
     * @note This will at most delete only one record.
     * @note This uses MongoCollection::remove() with justOne=TRUE
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
     * Map valid fields to an array of data.
     *
     * This is useful for mapping fields from a form, for example from $_POST, to
     * a pre-defined Struct, which can then be inserted into a collection.
     *
     * Map() reads the valid fields of a ValidationReport and matches them to
     * the elements of an array.  If a field name doesn't exist in the array,
     * it is skipped.
     *
     * An associative array of field name/element name overrides can also be
     * provided to rename fields in the returned array.  If the field name
     * is mapped to NULL, or to a field that doesn't exist, it is skipped, allowing
     * certain fields to be masked.
     *
     * @param ValidationReport $VR The ValidationReport to map.
     * @param array $Src Associative array of data to map.
     * @param array $Override Array of renamed or skipped field names.
     * @retval array A consisitent associative array of field names and their values.
     *
     * @note Use this method and custom Structs for guaranteed consistency and complex nested
     *       array structures when storing data in a mongo collection.
     * @note The structure of $Override (field name=>new name) is different from
     *       that of MySQL::Map() (column name=>element name).
     * @note While the overall ValidationReport isn't checked to be valid, it
     *       likely should be before using it in this method.
     * @note A valid field is one that's TRUE.  This may include an empty field
     *       which is not required.  All others are skipped, regardless of $Override.
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

    /**
     * Internal method for displaying debug information about the last CRUDC query.
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
}

