@@@ListProfiles
SELECT ProfileID,InsertDT,FirstName,LastName
  FROM Profile
 ORDER BY InsertDT DESC

@@@UpdateFirstName
UPDATE Profile SET FirstName=<?=$this($FirstName)?>
 WHERE ProfileID=<?=$this($ProfileID)?>

@@@InsertFileUpload
INSERT INTO FileUpload (R_ProfileID,Filename,FileSize,ContentType,FileData)
VALUES (<?=$this($R_ProfileID)?>,<?=$this($Filename)?>,<?=$this($FileSize)?>,
        <?=$this($ContentType)?>,<?=$this($FileData,'varbinary')?>);

@@@CreateProfileTable
CREATE TABLE Profile(
	ProfileID int IDENTITY(1,1) NOT NULL,
	InsertDT datetime NULL,
	Prefix varchar(45) NULL,
	FirstName varchar(45) NULL,
	LastName varchar(45) NULL,
	Suffix varchar(45) NULL,
	Address1 varchar(45) NULL,
	Address2 varchar(45) NULL,
	City varchar(45) NULL,
	State char(2) NULL,
	ZipCode varchar(45) NULL,
	Email varchar(45) NULL,
	Age int NULL,
	PhoneNumber varchar(45) NULL,
	CCNumber varchar(45) NULL,
	IP varchar(45) NULL,
	SSN varchar(45) NULL,
	Description varchar(500) NULL,
	Username varchar(45) NULL,
	Password varchar(45) NULL,
	HAU varchar(45) NULL,
    PRIMARY KEY (ProfileID)
)

@@@CreateFileUploadTable
CREATE TABLE FileUpload (
  FileID int IDENTITY(1,1) NOT NULL,
  R_ProfileID int NULL,
  Filename varchar(45) NULL,
  FileSize int NULL,
  ContentType varchar(45) NULL,
  FileData varbinary(MAX),
  PRIMARY KEY (FileID)
)











