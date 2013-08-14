<?php
namespace asm;

// manage asmblr Pack - a pack is a disk or memory representation of an entire site
// on disk, Site.inc, Pages/, Templates/, Content/
// in memory: array('Site'=>array(),'Pages'=>array(),'Templates'=>array(),'Content'=>array()
//        where Pages/Templates are associative arrays of structs by Name, Content is by Path
// there's no security here
class Pack
{
    protected static $Skel = array('Site'=>array(),'Pages'=>array(),'Templates'=>array(),'Content'=>array());

    public $ss;
    public $ps;
    public $ts;
    public $cs;


    // only need to instantiate for mongo - the rest is static
    public function __construct( \asm\SiteSet $ss,\asm\PageSet $ps,\asm\TemplateSet $ts,\asm\ContentSet $cs )
    {
        $this->ss = $ss;
        $this->ps = $ps;
        $this->ts = $ts;
        $this->cs = $cs;
    }

    // when Site has it's Config populated, we should not include Config on "export"
    public function FromMongo( \MongoId $Site_id,$NoContent = FALSE )
    {
        $Site = $this->ss->Read($Site_id);
        if( empty($Site) )
            throw new Exception('No site found for '.(string)$Site_id);

        $Pages = array();
        foreach( $this->ps->Listing($Site['_id']) as $P )
        {
            $this->ps->Flatten($P);
            $Pages[$P['Name']] = $P;
        }

        $Templates = array();
        foreach( $this->ts->Listing($Site['_id']) as $T )
            $Templates[$T['Name']] = $T;

        $Content = array();
        if( $NoContent === FALSE )
        {
            // stores only path and body
            // this, and what we use in ContentSrv, could be refactored into GetBody/SendBody for
            // unified delivery or saving of the content - this is basically duped from ContentSRv
            // it would also need to deal with S3/GAE in addition to Mongo
            foreach( $this->cs->Listing($Site_id,TRUE,TRUE) as $C )
                $Content[$C['Path']] = is_object($C['Body'])?$C['Body']->bin:$C['Body'];
        }

        $S = static::$Skel;
        $S['Site'] = $Site;
        $S['Pages'] = $Pages;
        $S['Templates'] = $Templates;
        $S['Content'] = $Content;

        return $S;
    }

    // if $Site['Site']['_id'] is empty, a new site is created, otherwise that _id is used and checked to exist
    // ALL EXISTING page/template are deleted from an existing site - content is only added - overwritten if dupe
    public function ToMongo( &$Site )
    {
        try
        {
            if( empty($Site['Site']['_id']) )
            {
                $S_id = $Site['Site']['Directives'];

                $this->ss->Create($Site['Site']);

                foreach( $S_id as $D )
                    $this->ss->SetDirective($D,$Site['Site']);
            }
            else
            {
                $S = $this->ss->Read($Site['Site']['_id']);
                if( empty($S) )
                    throw new Exception("Implied site doesn't exist: {$Site['Site']['_id']}");

                // is the format of directives the same across the board?  I don't think so
                foreach( $S['Directives'] as $_id =>$D )
                    $this->ss->DeleteDirective(new \MongoId($_id),$S);

                foreach( $Site['Site']['Directives'] as $D )
                    $this->ss->SetDirective($D,$S);

                // hackish
                if( !empty($Site['Site']['Routine'][0]) )
                    $this->ss->SetRoutine($Site['Site']['Routine'][0],$S);

                $Site['Site'] = $S;
            }
        }
        catch( \Exception $E )
        {
            if( $this->ss->IsDupeE($E) )
                throw new Exception('Duplicate Domain');
            else
                throw $E;
        }

        $this->ps->UseSite_id($Site['Site']['_id']);
        $this->ts->UseSite_id($Site['Site']['_id']);

        // delete all existing pages/templates/content
        foreach( $this->ps->Listing($Site['Site']['_id']) as $P )
            $this->ps->Delete($P);

        foreach( $this->ts->Listing($Site['Site']['_id']) as $T )
            $this->ts->Delete($T);

//         foreach( $this->cs->Listing($Site['Site']['_id']) as $C )
//             $this->cs->Delete($C);

        foreach( $Site['Pages'] as &$Page )
        {
            $Page['Site_id'] = $Site['Site']['_id'];
            $P_id = $Page['Directives'];
            $this->ps->Create($Page);

            foreach( $P_id as $D )
                $this->ps->SetDirective($D,$Page);
        }

        foreach( $Site['Templates'] as &$Template )
        {
            $Template['Site_id'] = $Site['Site']['_id'];
            $this->ts->Create($Template);
        }

        foreach( $Site['Content'] as &$Content )
        {
            $Content['Site_id'] = $Site['Site']['_id'];

            try
            {
                $this->cs->Create($Content);
            }
            catch( \Exception $E )
            {
                if( $this->cs->IsDupeE($E) )
                {
                    $C = $this->cs->Match($Content['Site_id'],$Content['Path']);
                    if( !empty($C) )
                        $this->cs->SetBody($Content['Body'],$C);
                }
                else
                    throw $E;
            }

        }
    }

    // ignores all files/directories except known ones and ending in .inc (though everything in Content/)
    // the site _id here are bogus - they're all the Account_id so Site['_id'] needs to be set in the returned value
    // filenames dictate page/template/content names
    public static function FromDir( $Dir,$Domain,\MongoId $Account_id )
    {
        $RealDir = realpath($Dir);
        if( empty($RealDir) )
            throw new Exception("Dir '$Dir' is not readable.");

        if( !is_readable("{$RealDir}/Site.inc") )
            throw new Exception("Dir '$RealDir' doesn't contain a Site.inc.");

        $Site = Site::Init($Domain,$Account_id);
        Site::FromPHP(file_get_contents("{$RealDir}/Site.inc"),$Site);

        $Pages = array();
        if( ($LS = @scandir("{$RealDir}/Pages")) !== FALSE )
        {
            foreach( $LS as $File )
            {
                if( $File{0} === '.' || strpos($File,'.inc') === FALSE )
                    continue;

                $FilePath = "{$RealDir}/Pages".DIRECTORY_SEPARATOR.$File;

                $Page = Page::Init($Account_id,str_replace('.inc','',$File),'/'.microtime());
                Page::FromPHP(file_get_contents($FilePath),$Page);
                $Pages[$Page['Name']] = $Page;
            }
        }

        $Templates = array();
        if( ($LS = @scandir("{$RealDir}/Templates")) !== FALSE )
        {
            foreach( $LS as $File )
            {
                if( $File{0} === '.' || strpos($File,'.inc') === FALSE )
                    continue;

                $FilePath = "{$RealDir}/Templates".DIRECTORY_SEPARATOR.$File;

                $Template = Template::Init($Account_id,str_replace('.inc','',$File));
                Template::FromPHP(file_get_contents($FilePath),$Template);
                $Templates[$Template['Name']] = $Template;
            }
        }

        $Content = array();
        // need to recursive for sub-dirs?
        if( ($LS = @scandir("{$RealDir}/Content")) !== FALSE )
        {
            foreach( $LS as $File )
            {
                if( $File{0} === '.' )
                    continue;

                $FilePath = "{$RealDir}/Content".DIRECTORY_SEPARATOR.$File;

                $Con = Content::Init($Account_id,$File,file_get_contents($FilePath));
                $Content[$Con['Path']] = $Con;
            }
        }

        $S = static::$Skel;
        $S['Site'] = $Site;
        $S['Pages'] = $Pages;
        $S['Templates'] = $Templates;
        $S['Content'] = $Content;

        return $S;
    }

    // directory is created if needed
    // existing files with the same name are overwritten - other files/dirs aren't touched
    public static function ToDir( $Site,$Dir )
    {
        $RealDir = realpath($Dir);
        if( empty($RealDir) )
        {
            if( !mkdir("/{$Dir}") )
                throw new Exception("Unable to create directory '/{$Dir}'");

            $RealDir = realpath($Dir);
        }
        else if( !is_writable($RealDir) )
            throw new Exception("Can't write to existing dir: $RealDir");

        foreach( array('Pages','Templates','Content') as $D )
        {
            if( !is_dir("{$RealDir}/{$D}") )
                if( !mkdir("{$RealDir}/{$D}") )
                    throw new Exception("Unable to create '{$RealDir}/{$D}'");
        }

        file_put_contents("{$RealDir}/Site.inc",Site::ToPHP($Site['Site']));

        foreach( $Site['Pages'] as $P )
            file_put_contents("{$RealDir}/Pages/{$P['Name']}.inc",Page::ToPHP($P));

        foreach( $Site['Templates'] as $T )
            file_put_contents("{$RealDir}/Templates/{$T['Name']}.inc",Template::ToPHP($T));

        // this may not work if there's dir structure in the content name
        foreach( $Site['Content'] as $Name => $Body )
            file_put_contents("{$RealDir}/Content/{$Name}",$Body);
    }

    // similar to FromDir
    public static function FromZip( $ZipFilePath,$Domain,\MongoId $Account_id )
    {
        $Z = new \ZipArchive;
        if( $Z->open($ZipFilePath) !== TRUE )
            throw new Exception("Failed to read zip file: $ZipFilePath");

        $Site = $Pages = $Templates = $Content = array();
        for( $i = 0; $i < $Z->numFiles; ++$i )
        {
            $Filename = $Z->statIndex($i)['name'];

            if( $Filename === 'Site.inc' )
            {
                $Site = Site::Init($Domain,$Account_id);
                Site::FromPHP($Z->getFromIndex($i),$Site);
            }
            else
            {
                $T = \fw\Path::Init($Filename);
                if( \fw\Path::Top($T) === 'Pages' && $Filename !== 'Pages/' )
                {
                    $Page = Page::Init($Account_id,str_replace('.inc','',\fw\Path::Bottom($T)),'/'.microtime());
                    Page::FromPHP($Z->getFromName($Filename),$Page);
                    $Pages[$Page['Name']] = $Page;
                }
                else if( \fw\Path::Top($T) === 'Templates' && $Filename !== 'Templates/' )
                {
                    $Template = Template::Init($Account_id,str_replace('.inc','',\fw\Path::Bottom($T)));
                    Template::FromPHP($Z->getFromName($Filename),$Template);
                    $Templates[$Template['Name']] = $Template;
                }
                else if( \fw\Path::Top($T) === 'Content' && $Filename !== 'Content/' )
                {
                    $Con = Content::Init($Account_id,str_replace('Content/','',$Filename),$Z->getFromIndex($i));
                    $Content[$Con['Path']] = $Con;
                }
            }
        }

        $S = static::$Skel;
        $S['Site'] = $Site;
        $S['Pages'] = $Pages;
        $S['Templates'] = $Templates;
        $S['Content'] = $Content;

        return $S;
    }

    // similar to ToDir
    public static function ToZip( $Site,$ZipFilePath )
    {
        $Z = new \ZipArchive;
        if( $Z->open($ZipFilePath,\ZIPARCHIVE::CREATE) !== TRUE )
            throw new Exception("Failed to create tmp zip file at {$ZipFilePath}.");

        $Z->addEmptyDir('Pages');
        $Z->addEmptyDir('Templates');
        $Z->addEmptyDir('Content');

        $Z->addFromString('Site.inc',Site::ToPHP($Site['Site']));

        foreach( $Site['Pages'] as $P )
            $Z->addFromString("Pages/{$P['Name']}.inc",Page::ToPHP($P));

        foreach( $Site['Templates'] as $T )
            $Z->addFromString("Templates/{$T['Name']}.inc",Template::ToPHP($T));

        // this may not work if there's dir structure in the content name
        foreach( $Site['Content'] as $Name => $Body )
            $Z->addFromString("Content/{$Name}",$Body);

        $Z->close();
    }
}


