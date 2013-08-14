<?php
namespace asm;

// manage versions of sites in svn/git along with help from Pack
class Versioning
{
    protected $BranchURL = '';
    protected $TrunkURL = '';
    protected $Account_id = '';
    public $ss;
    public $ps;
    public $ts;
    public $cs;


    public function __construct( $BranchURL,$TrunkURL,\MongoId $Account_id,\asm\SiteSet $ss,\asm\PageSet $ps,\asm\TemplateSet $ts,\asm\ContentSet $cs )
    {
        $this->BranchURL = $BranchURL;
        $this->TrunkURL = $TrunkURL;
        $this->Account_id = $Account_id;

        $this->ss = $ss;
        $this->ps = $ps;
        $this->ts = $ts;
        $this->cs = $cs;
    }

    // completly reverts site to what's in svn - local changes are lost!
    public function svnRevert( \MongoId $Site_id )
    {
        $Site = $this->ss->Read($Site_id);
        if( empty($Site) )
            return FALSE;

        $exportDir = sys_get_temp_dir()."/{$Site['Domain']}_export_".time().'/';

        $this->svnExec("export {$this->BranchURL} {$exportDir}");

        $S = Pack::FromDir($exportDir,$Site['Domain'],$this->Account_id);

        $S['Site']['_id'] = $Site['_id'];

        $pack = new Pack($this->ss,$this->ps,$this->ts,$this->cs);
        $pack->ToMongo($S);

        // yup
        system("rm -rf {$exportDir}");
    }

    // commit changes to svn
    // this uses a lot of RAM
    public function svnCommit( \MongoId $Site_id,$NoContent = FALSE )
    {
        $Site = $this->ss->Read($Site_id);
        if( empty($Site) )
            return FALSE;

        $coDir = sys_get_temp_dir()."/{$Site['Domain']}_co_".time();

        $this->svnExec("co {$this->BranchURL} {$coDir}");
        $this->svnExec("merge {$this->TrunkURL} {$coDir}");
        $this->svnExec("ci {$coDir} -m 'merge of trunk to {$Site['Domain']} branch'");

        // to determine what's we may have deleted locally - not very efficient
        $BranchS = Pack::FromDir($coDir,$Site['Domain'],$Site['AAID']);

        // now merge pages/templates/content from our local site in mongo - svn add new files
        $pack = new Pack($this->ss,$this->ps,$this->ts,$this->cs);
        $asmS = $pack->FromMongo($Site['_id'],$NoContent);

        // save a snapshot backup just in case
        Pack::ToDir($asmS,$coDir."_snapshot");

        file_put_contents("{$coDir}/Site.inc",Site::ToPHP($asmS['Site']));

        foreach( $asmS['Pages'] as $P )
        {
            $FN = "{$coDir}/Pages/{$P['Name']}.inc";

            $Exists = is_file($FN);
            file_put_contents($FN,Page::ToPHP($P));

            if( !$Exists )
                $this->svnExec("add $FN");
        }

        foreach( $asmS['Templates'] as $T )
        {
            $FN = "{$coDir}/Templates/{$T['Name']}.inc";

            $Exists = is_file($FN);
            file_put_contents($FN,Template::ToPHP($T));

            if( !$Exists )
                $this->svnExec("add $FN");
        }

        // common problem of dir structures here too
        foreach( $asmS['Content'] as $Name => $Body )
        {
            $FN = "{$coDir}/Content/{$Name}.inc";

            $Exists = is_file($FN);
            file_put_content($FN,$Body);

            if( !$Exists )
                $this->svnExec("add $FN");
        }

        // delete items that were deleted locally
        foreach( $BranchS['Pages'] as $P )
            if( !isset($asmS['Pages'][$P['Name']]) )
                $this->svnExec("rm {$coDir}/Pages/{$P['Name']}.inc");

        foreach( $BranchS['Templates'] as $T )
            if( !isset($asmS['Templates'][$T['Name']]) )
                $this->svnExec("rm {$coDir}/Templates/{$T['Name']}.inc");

        foreach( $BranchS['Content'] as $Name => $Body )
            if( !isset($asmS['Content'][$Name]) )
                $this->svnExec("rm {$coDir}/Content/{$Name}.inc");


        $this->svnExec("ci {$coDir} -m 'asm local commit of {$Site['Domain']}'");

        // yup
        system("rm -rf {$coDir}");
    }


    // yay - and no escaping so don't be dumb
    protected function svnExec( $Cmd )
    {
        $R = 0;
        $OP = array();
        exec("echo 't' | /usr/bin/svn -q --config-dir /tmp/svn.silly --config-option servers:global:store-auth-creds=no --username praxis --password st4ckw4r3 $Cmd 2>&1",$OP,$R);
        if( $R !== 0 )
            throw new Exception("svn {$Cmd} failed: ".implode("\n",$OP));
    }

}

