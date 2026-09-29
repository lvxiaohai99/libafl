# LTE-V2X / CCSA 安全消息 ASN.1 规范汇总

> 由 `demo/v2x_sec/ccsaasn1/src/` 下 ASN.1 源文件整理，便于语雀导入。
>
> - 工具链：asn1c 0.9.29（`-no-gen-example -fcompound-names`）
> - 生成产物目录：`ccsaasn1/out/`
> - 未纳入：`v2x_sec.asn`（SD 合并稿，与 `Security*` 模块类型重名，请单独查阅）

## 文件目录

### 基础数据类型

- `SecurityBaseDataTypes.asn` → 模块 `SecurityBaseDataTypes`
- `SecurityDataTypes.asn` → 模块 `SecurityDataTypes`

### 安全消息封装

- `V2XSecData-Encrypted.asn` → 模块 `V2XSecData-Encrypted`
- `ScmsPdu.asn` → 模块 `ScmsPdu`
- `ScmsSsp.asn` → 模块 `ScmsSsp`

### 证书与实体接口 PDU

- `AcaEeCertResponseCubkSpdu.asn` → 模块 `AcaEeCertResponseCubkSpdu`
- `AcaEeInterfacePdu.asn` → 模块 `AcaEeInterfacePdu`
- `AcaRaInterfacePdu.asn` → 模块 `AcaRaInterfacePdu`
- `EcaEeInterfacePdu.asn` → 模块 `EcaEeInterfacePdu`
- `EeRaInterfacePdu.asn` → 模块 `EeRaInterfacePdu`
- `LaMaInterfacePdu.asn` → 模块 `LaMaInterfacePdu`
- `LaRaInterfacePdu.asn` → 模块 `LaRaInterfacePdu`
- `CertManagementPdu.asn` → 模块 `CertManagementPdu`

### 信任链与吊销（CTL / CRL）

- `CTL.asn` → 模块 `CTL`
- `CRLBaseTypes.asn` → 模块 `CRLBaseTypes`
- `CRLSecurityPacage.asn` → 模块 `SecuredCrl`
- `CRLSSP.asn` → 模块 `CrlSsp`

### 厂商扩展

- `XDJA-EC-PC.asn` → 模块 `CCSA`

## 模块依赖关系（简图）

```text
SecurityBaseDataTypes
        │
        ▼
SecurityDataTypes
        │
        ├──────────────► V2XSecData-Encrypted
        │                        │
        │                        ├─► AcaEeCertResponseCubkSpdu
        │                        ├─► XDJA-EC-PC（厂商加密 SPDU）
        │                        └─► La*/Ee*/Aca*/Eca* InterfacePdu
        │
        ├─► ScmsPdu / ScmsSsp / CertManagementPdu
        └─► CTL / CRLBaseTypes / SecuredCrl(CRLSecurityPacage) / CrlSsp
```

---

## SecurityBaseDataTypes.asn

- **ASN.1 模块名**：`SecurityBaseDataTypes`
- **（无 IMPORTS）**

```asn1
--********************************************************************
--  LTE-V2X Security Base Data Types 
-- 
--******************************************************************** 
SecurityBaseDataTypes DEFINITIONS AUTOMATIC TAGS ::= BEGIN
EXPORTS ALL;
-- ------------------------------------------------------------------
-- 
-- Integers
--
-- ------------------------------------------------------------------


Uint3  ::= INTEGER (0..7)                    -- (hex)                      07
Uint8  ::= INTEGER (0..255)                  -- (hex)                      ff
Uint16 ::= INTEGER (0..65535)                -- (hex)                   ff ff
Uint32 ::= INTEGER (0..4294967295)--<LONGLONG>-- -- (hex)             ff ff ff ff
Uint64 ::= INTEGER (0..18446744073709551615) -- (hex) ff ff ff ff ff ff ff ff

SequenceOfUint8  ::= SEQUENCE OF Uint8
SequenceOfUint16 ::= SEQUENCE OF Uint16

-- ------------------------------------------------------------------
-- 
-- OCTET STRING types
--
-- ------------------------------------------------------------------


Opaque ::= OCTET STRING

 HashedId32 ::= OCTET STRING (SIZE(32))
HashedId10 ::= OCTET STRING (SIZE(10))
HashedId8  ::= OCTET STRING (SIZE(8))
HashedId4  ::= OCTET STRING (SIZE(4))
HashedId3  ::= OCTET STRING (SIZE(3))
SequenceOfHashedId3 ::= SEQUENCE OF HashedId3


-- ------------------------------------------------------------------
-- 
-- Time
--
-- ------------------------------------------------------------------

Time32 ::= Uint32
Time64 ::= Uint64

ValidityPeriod ::= SEQUENCE  {
    start          Time32,
    duration       Duration
}

Duration ::= CHOICE  {
    microseconds     Uint16,
    milliseconds     Uint16,
    seconds          Uint16,
    minutes          Uint16,
    hours            Uint16,
    sixtyHours       Uint16,
    years            Uint16
} 

-- ------------------------------------------------------------------
-- 
-- Location
--
-- ------------------------------------------------------------------


GeographicRegion ::= CHOICE  {
    circularRegion      CircularRegion,
    rectangularRegion   SequenceOfRectangularRegion,
    polygonalRegion     PolygonalRegion,
    identifiedRegion    SequenceOfIdentifiedRegion,
    ...
}

CircularRegion ::= SEQUENCE  {
    center          TwoDLocation,
    radius          Uint16
}

RectangularRegion ::= SEQUENCE  {
    northWest       TwoDLocation,
    southEast       TwoDLocation
}

SequenceOfRectangularRegion ::= SEQUENCE OF RectangularRegion

PolygonalRegion ::= SEQUENCE SIZE(3..MAX) OF TwoDLocation

TwoDLocation ::= SEQUENCE  {
    latitude      Latitude,
    longitude     Longitude
}

IdentifiedRegion ::= CHOICE  {
    countryOnly           CountryOnly,
    countryAndRegions     CountryAndRegions,
    countryAndSubregions   CountryAndSubregions,
    ...
}

SequenceOfIdentifiedRegion ::= SEQUENCE OF IdentifiedRegion

CountryOnly ::= Uint16

CountryAndRegions ::= SEQUENCE  {
    countryOnly    CountryOnly,
    regions        SequenceOfUint8
}

CountryAndSubregions ::= SEQUENCE  {
    country             CountryOnly,
    regionAndSubregions  SequenceOfRegionAndSubregions
}

RegionAndSubregions ::= SEQUENCE  {
    region             Uint8,
    subregions         SequenceOfUint16
}

SequenceOfRegionAndSubregions ::= SEQUENCE OF RegionAndSubregions

ThreeDLocation ::= SEQUENCE  {
    latitude        Latitude,
    longitude       Longitude,
    elevation       Elevation
}

Latitude ::= NinetyDegreeInt
Longitude ::= OneEightyDegreeInt
Elevation ::= ElevInt

NinetyDegreeInt ::= INTEGER  {
    min 		(-900000000),
    max 		(900000000),
    unknown 	(900000001)
} (-900000000..900000001)

KnownLatitude ::= NinetyDegreeInt (min..max) -- Minus 90deg to +90deg in microdegree intervals
UnknownLatitude ::= NinetyDegreeInt (unknown)
  
OneEightyDegreeInt ::= INTEGER  {
    min      	(-1799999999),
    max      	(1800000000),
    unknown  	(1800000001)
} (-1799999999..1800000001)

KnownLongitude ::= OneEightyDegreeInt (min..max)
UnknownLongitude ::= OneEightyDegreeInt (unknown)
  
ElevInt ::= Uint16 -- Range is from -4096 to 61439 in units of one-tenth of a meter

-- ------------------------------------------------------------------
-- 
-- Crypto
--
-- ------------------------------------------------------------------

Signature ::= CHOICE  {
    ecdsaNistP256Signature        EcdsaP256Signature,
    ecdsaBrainpoolP256r1Signature EcdsaP256Signature,
    ...,
    ecdsaBrainpoolP384r1Signature EcdsaP384Signature,
    ecdsaNistP384Signature     EcdsaP384Signature,
    sm2Signature                  EcsigP256Signature
}

EcdsaP256Signature ::= SEQUENCE  {
    rSig     EccP256CurvePoint,
    sSig     OCTET STRING (SIZE (32))
}

EcsigP256Signature ::= SEQUENCE {
    rSig OCTET STRING (SIZE (32)),
    sSig OCTET STRING (SIZE (32))
}

EcdsaP384Signature ::= SEQUENCE  {
    rSig     EccP384CurvePoint,
    sSig     OCTET STRING (SIZE (48))
}

EccP256CurvePoint ::= CHOICE  {
    x-only           OCTET STRING (SIZE (32)),
    fill             NULL,  -- consistency with 1363 / X9.62
    compressed-y-0   OCTET STRING (SIZE (32)),
    compressed-y-1   OCTET STRING (SIZE (32)),
    uncompressed     SEQUENCE  {
        x OCTET STRING (SIZE (32)),
        y OCTET STRING (SIZE (32))
    }
}

  EccP384CurvePoint::= CHOICE  {
      x-only          OCTET STRING (SIZE (48)),
      fill            NULL, -- consistency w 1363 / X9.62
      compressed-y-0  OCTET STRING (SIZE (48)),
      compressed-y-1  OCTET STRING (SIZE (48)),
      uncompressed    SEQUENCE  {
          x OCTET STRING (SIZE (48)),
          y OCTET STRING (SIZE (48))
      }
  }


SymmAlgorithm ::= ENUMERATED  { 
    aes128Ccm,
    ...,
    sm4Ccm
}

HashAlgorithm ::= ENUMERATED  { 
    sha256,
    ...,
    sha384,
    sm3
}

EciesP256EncryptedKey ::= SEQUENCE  {
    v               EccP256CurvePoint,
    c               OCTET STRING (SIZE (16)),
    t               OCTET STRING (SIZE (16))
}

EcencP256EncryptedKey ::= SEQUENCE  {
      v               EccP256CurvePoint,
      t               OCTET STRING (SIZE (32)),
      c               OCTET STRING (SIZE (16))
  }

EncryptionKey ::= CHOICE  {
    public               PublicEncryptionKey,
    symmetric            SymmetricEncryptionKey 
}

PublicEncryptionKey ::= SEQUENCE  { 
    supportedSymmAlg      SymmAlgorithm,
    publicKey             BasePublicEncryptionKey
}

BasePublicEncryptionKey ::= CHOICE  { 
    eciesNistP256             EccP256CurvePoint,
    eciesBrainpoolP256r1      EccP256CurvePoint,
    ...,
    ecencSm2                  EccP256CurvePoint
}

PublicVerificationKey ::= CHOICE  { 
    ecdsaNistP256             EccP256CurvePoint,
    ecdsaBrainpoolP256r1      EccP256CurvePoint,
    ...,
    ecdsaBrainpoolP384r1      EccP384CurvePoint,
    ecdsaNistP384          EccP384CurvePoint,
    ecsigSm2                  EccP256CurvePoint
}

SymmetricEncryptionKey ::= CHOICE  {
    aes128Ccm     OCTET STRING(SIZE(16)),
    ...,
    sm4Ccm        OCTET STRING(SIZE(16))
}

-- ------------------------------------------------------------------
-- 
-- AID / ITS-AID
--
-- ------------------------------------------------------------------


AidSsp ::= SEQUENCE  {
    aid                  Aid,
    ssp                   ServiceSpecificPermissions OPTIONAL
}

SequenceOfAidSsp ::= SEQUENCE OF AidSsp

Aid ::= INTEGER (0..MAX)

SequenceOfAid ::= SEQUENCE OF Aid

ServiceSpecificPermissions ::= CHOICE  {
    opaque              OCTET STRING (SIZE(0..MAX)),
    ...,
    bitmapSsp           BitmapSsp
}

BitmapSsp ::= OCTET STRING (SIZE(0..31))

SspValue ::= OCTET STRING (SIZE(0..31))

SspBitmask ::= OCTET STRING (SIZE(0..31))

AidSspRange ::= SEQUENCE  {
    aid                Aid,
    sspRange            SspRange OPTIONAL
}

SequenceOfAidSspRange ::= SEQUENCE OF AidSspRange

SspRange ::= CHOICE  {
    opaque              SequenceOfOctetString,
    all                 NULL,
    ... ,
    bitmapSspRange      BitmapSspRange
}
   
BitmapSspRange ::= SEQUENCE  {
    sspValue            OCTET STRING (SIZE(1..32)),
    sspBitmask          OCTET STRING (SIZE(1..32))
}

SequenceOfOctetString ::= SEQUENCE (SIZE (0..MAX)) OF 
    OCTET STRING (SIZE(0..MAX))


-- ------------------------------------------------------------------
-- 
-- Goes in certs
--
-- ------------------------------------------------------------------

SubjectAssurance ::= OCTET STRING (SIZE(1))

CrlSeries ::= Uint16
  

-- ------------------------------------------------------------------
-- 
-- Pseudonym Linkage
--
-- ------------------------------------------------------------------

IValue ::= Uint16
Hostname ::= UTF8String (SIZE(0..255))
LinkageValue ::= OCTET STRING (SIZE(9))
GroupLinkageValue ::= SEQUENCE  {
    jValue  OCTET STRING (SIZE(4)),
    value   OCTET STRING (SIZE(9))
}
  
LaId ::= OCTET STRING (SIZE(2)) 
LinkageSeed ::= OCTET STRING (SIZE(16))

END
```

---

## SecurityDataTypes.asn

- **ASN.1 模块名**：`SecurityDataTypes`
- **依赖模块：SecurityBaseDataTypes**

```asn1
--******************************************************************************
-- 
-- LTE-V2X Security Data Types 
-- 
--******************************************************************************

SecurityDataTypes DEFINITIONS AUTOMATIC TAGS ::= BEGIN 
 
EXPORTS ALL;

IMPORTS 
  CrlSeries,
  EccP256CurvePoint,
  EciesP256EncryptedKey,
  EcencP256EncryptedKey,
  EncryptionKey,
  GeographicRegion,
  GroupLinkageValue,
  HashAlgorithm,
  HashedId3,
  HashedId8,
  HashedId32,
  Hostname,
  IValue,
  LinkageValue,
  Opaque,
  Aid,
  AidSsp,
  AidSspRange,
  PublicEncryptionKey,
  PublicVerificationKey,
  SequenceOfHashedId3,
  SequenceOfAidSsp,
  SequenceOfAidSspRange,
  ServiceSpecificPermissions,
  Signature,
  SubjectAssurance,
  SymmetricEncryptionKey,
  ThreeDLocation,
  Time64,
  Uint3,
  Uint8,
  Uint16, 
  Uint32,
  ValidityPeriod FROM SecurityBaseDataTypes

;
-- 
--********************************************************************* 
-- 
--  Structures for describing secured data
-- 
--*********************************************************************  

-- this structure belongs later in the file but putting it here avoids
-- compiler errors with certain tools
SignedDataPayload ::= SEQUENCE  { 
    data              V2XSecData OPTIONAL,
    extDataHash       HashedData OPTIONAL,
    ...
}
    (WITH COMPONENTS {..., data PRESENT} |
     WITH COMPONENTS {..., extDataHash PRESENT})

V2XSecData ::= SEQUENCE  {
    protocolVersion    Uint8(3),
    content            V2XSecContent
}

V2XSecContent ::=  CHOICE  { 
    unsecuredData            Opaque, 
    signedData               SignedData,
    encryptedData            EncryptedData,
    signedCertificateRequest Opaque,
    ...
}

SignedData ::= SEQUENCE  { 
    hashId          HashAlgorithm,
    tbsData         ToBeSignedData,
    signer          SignerIdentifier,
    signature       Signature
}


SignerIdentifier ::= CHOICE  { 
    digest          HashedId8,
    certificate     SequenceOfCertificate,
    self            NULL,
    ...,
    x509           OCTET STRING
}

ToBeSignedData ::= SEQUENCE  { 
    payload         SignedDataPayload,
    headerInfo      HeaderInfo
}

HashedData::= CHOICE  { 
    sha256HashedData  OCTET STRING (SIZE(32)),
...,
sha384HashedData  OCTET STRING (SIZE(48)),
sm3HashedData  OCTET STRING (SIZE(32))
}

HeaderInfo ::= SEQUENCE  { 
    aid                   Aid,
    generationTime         Time64 OPTIONAL,
    expiryTime             Time64  OPTIONAL,
    generationLocation     ThreeDLocation OPTIONAL,
    p2pcdLearningRequest   HashedId3 OPTIONAL,
    missingCrlIdentifier   MissingCrlIdentifier OPTIONAL,
    encryptionKey          EncryptionKey OPTIONAL,
    ...,
    inlineP2pcdRequest     SequenceOfHashedId3 OPTIONAL,
    requestedCertificate   Certificate OPTIONAL,
    pduFunctionalType      PduFunctionalType OPTIONAL
}

MissingCrlIdentifier ::= SEQUENCE  {
    cracaId         HashedId3,
    crlSeries       CrlSeries,
    ...
}

PduFunctionalType ::= INTEGER (0..255)
  tlsHandshake           PduFunctionalType ::= 1
  iso21177ExtendedAuth   PduFunctionalType ::= 2


Countersignature ::= V2XSecData (WITH COMPONENTS {...,
  content (WITH COMPONENTS {..., 
    signedData  (WITH COMPONENTS {..., 
      tbsData (WITH COMPONENTS {..., 
        payload (WITH COMPONENTS {..., 
          data ABSENT,
          extDataHash PRESENT
        }),
        headerInfo(WITH COMPONENTS {..., 
          generationTime PRESENT,
          expiryTime ABSENT,
          generationLocation ABSENT,
          p2pcdLearningRequest ABSENT,
          missingCrlIdentifier ABSENT,
          encryptionKey ABSENT
        })
      })
    })
  })
})

--********************************************************************** 
-- 
--  Structures for describing encrypted data
-- 
--**********************************************************************  

EncryptedData ::= SEQUENCE  {
    recipients     SequenceOfRecipientInfo,
    ciphertext     SymmetricCiphertext
}
RecipientInfo ::= CHOICE  {
    pskRecipInfo        PreSharedKeyRecipientInfo,
    symmRecipInfo       SymmRecipientInfo,
    certRecipInfo       PKRecipientInfo, 
    signedDataRecipInfo PKRecipientInfo, 
    rekRecipInfo        PKRecipientInfo 
}

SequenceOfRecipientInfo ::= SEQUENCE OF RecipientInfo
       
PreSharedKeyRecipientInfo ::= HashedId8
SymmRecipientInfo ::= SEQUENCE  { 
    recipientId        HashedId8, 
    encKey             SymmetricCiphertext
}

PKRecipientInfo ::= SEQUENCE  { 
    recipientId        HashedId8, 
    encKey             EncryptedDataEncryptionKey
}

EncryptedDataEncryptionKey ::= CHOICE  { 
    eciesNistP256          EciesP256EncryptedKey,
    eciesBrainpoolP256r1   EciesP256EncryptedKey,
...,
    ecencSm2             EcencP256EncryptedKey
}

  SymmetricCiphertext ::= CHOICE  {
      aes128ccm           One28BitCcmCiphertext,
      ...,
      sm4Ccm              One28BitCcmCiphertext
  }

  One28BitCcmCiphertext ::= SEQUENCE  {
      nonce           OCTET STRING (SIZE (12)),
      ccmCiphertext   Opaque 
  }

--********************************************************************** 
-- 
--  Certificates and other security management data structures
-- 
--********************************************************************** 

-- Certificates is explicit (type = explicit,
-- toBeSigned includes verification key, signature present) or Reserved ��Reserved for impilict cert��.

Certificate ::= CertificateBase (ExplicitCertificate | Reserved)

SequenceOfCertificate ::= SEQUENCE OF Certificate

CertificateBase ::= SEQUENCE  {
    version                 Uint8(3),
    type                    CertificateType,
    issuer                  IssuerIdentifier,
    toBeSigned              ToBeSignedCertificate,
    signature               Signature OPTIONAL
}
  
CertificateType  ::= ENUMERATED  {
    explicit,
    reserved,
...
}


ExplicitCertificate ::= CertificateBase (WITH COMPONENTS {...,
    type(explicit),
    toBeSigned(WITH COMPONENTS {..., 
        verifyKeyIndicator(WITH COMPONENTS {verificationKey})
    }), 
    signature PRESENT
    })

Reserved ::= CertificateBase (WITH COMPONENTS {...,
    type(reserved),
    toBeSigned(WITH COMPONENTS {..., 
      verifyKeyIndicator(WITH COMPONENTS {reconstructionValue})
    }), 
    signature ABSENT
    })

IssuerIdentifier ::= CHOICE  { 
    sha256AndDigest         HashedId8,
    self                    HashAlgorithm,
    ...,
    sha384AndDigest         HashedId8,
      sm3AndDigest            HashedId8

}

ToBeSignedCertificate ::= SEQUENCE  { 
    id                     CertificateId,
    cracaId                HashedId3,
    crlSeries              CrlSeries,
    validityPeriod         ValidityPeriod,
    region                 GeographicRegion OPTIONAL,
    assuranceLevel         SubjectAssurance OPTIONAL,
    appPermissions         SequenceOfAidSsp OPTIONAL,
    certIssuePermissions   SequenceOfAidGroupPermissions OPTIONAL,
    certRequestPermissions SequenceOfAidGroupPermissions OPTIONAL, 
    canRequestRollover     NULL OPTIONAL,
    encryptionKey          PublicEncryptionKey OPTIONAL,
    verifyKeyIndicator     VerificationKeyIndicator,
    ...
}
(WITH COMPONENTS { ..., appPermissions PRESENT} |
 WITH COMPONENTS { ..., certIssuePermissions PRESENT} |
 WITH COMPONENTS { ..., certRequestPermissions PRESENT})

CertificateId ::= CHOICE  {
    linkageData             LinkageData,
    name                    Hostname,
    binaryId                OCTET STRING(SIZE(1..64)),
    none                    NULL,
    ...
}

LinkageData ::= SEQUENCE  {
    iCert                 IValue,
    linkage-value         LinkageValue, 
    group-linkage-value   GroupLinkageValue OPTIONAL
}
         
EndEntityType ::= BIT STRING {app (0), enroll (1) } (SIZE (8)) (ALL EXCEPT {})

AidGroupPermissions ::= SEQUENCE  {
    subjectPermissions SubjectPermissions,
    minChainLength     INTEGER DEFAULT 1,
    chainLengthRange   INTEGER DEFAULT 0, 
    eeType             EndEntityType DEFAULT {app}
}

SequenceOfAidGroupPermissions ::= SEQUENCE OF AidGroupPermissions
SubjectPermissions ::= CHOICE  {
    explicit        SequenceOfAidSspRange,
    all             NULL,
    ...
}

VerificationKeyIndicator ::= CHOICE  {
    verificationKey         PublicVerificationKey,
    reconstructionValue     EccP256CurvePoint,
    ...
}
  
END
```

---

## V2XSecData-Encrypted.asn

- **ASN.1 模块名**：`V2XSecData-Encrypted`
- **依赖模块：SecurityBaseDataTypes、SecurityDataTypes、ScmsPdu、EcaEeInterfacePdu、AcaRaInterfacePdu**

```asn1
V2XSecData-Encrypted DEFINITIONS AUTOMATIC TAGS ::= BEGIN 

EXPORTS ALL;
IMPORTS Uint8,Time32 FROM SecurityBaseDataTypes
	Certificate FROM SecurityDataTypes
	V2XSecData FROM SecurityDataTypes
	ScmsPdu FROM ScmsPdu
	eeEcaCertRequest FROM EcaEeInterfacePdu
	AcaRaInterfacePdu FROM AcaRaInterfacePdu
	EcaEeInterfacePdu FROM EcaEeInterfacePdu
;
 V2XSecData-Encrypted {Tbe} ::= V2XSecData (WITH COMPONENTS {
    ...,
    content (WITH COMPONENTS {
      encryptedData (CONSTRAINED BY {
        --encryption of-- Tbe
      })
    })
  })
  V2XSecData-EncryptedOpen ::= V2XSecData (WITH COMPONENTS {
    ...,
    content (WITH COMPONENTS {
      encryptedData
    })
  })
 V2XSecData-EncryptedSigned {Tbes, Aid} ::= V2XSecData-Signed {
    V2XSecData-Encrypted {
      Tbes
    }, 
    Aid
  }
 V2XSecData-Signed {Tbs, Aid} ::= V2XSecData (WITH COMPONENTS {
    ...,
    content (WITH COMPONENTS {
      ...,
      signedData (WITH COMPONENTS {
        ...,
        tbsData (WITH COMPONENTS {
          ...,
          payload (WITH COMPONENTS {
            ...,
            data (WITH COMPONENTS {
              ...,
              content (WITH COMPONENTS {
                unsecuredData (CONTAINING Tbs)
              })
            })
          }),
          headerInfo (WITH COMPONENTS {
            ...,
            aid (Aid),
            generationTime ABSENT,
            expiryTime ABSENT,
            generationLocation ABSENT,
            p2pcdLearningRequest ABSENT,
            missingCrlIdentifier ABSENT,
            encryptionKey ABSENT
          })
        }),
        signer (SignerSingleCert)
      })
    })
  })
  V2XSecData-SignedCertRequest {Tbscr, Signer} ::= 
  V2XSecData (WITH COMPONENTS {
    ...,
    content (WITH COMPONENTS {
    ...,
      signedCertificateRequest (CONTAINING 
      SignedCertificateRequest (WITH COMPONENTS {
        ...,
        tbsRequest (Tbscr),
        signer (Signer)
      }))
    })
  })
 V2XSecData-SignedEncrypted {Tbse, Aid} ::= V2XSecData-Encrypted {
    V2XSecData-Signed {
      Tbse,
      Aid
    }
  }
 V2XSecData-SignedEncryptedCertRequest {Tbstecr, Signer} ::= 
  V2XSecData-Encrypted {
    V2XSecData-SignedCertRequest {
      Tbstecr, 
      Signer
    }
  }
 V2XSecData-Unsecured {Tbu} ::= V2XSecData (WITH COMPONENTS {
    content (WITH COMPONENTS {
      ...,
      unsecuredData (CONTAINING Tbu)
    })
  })
 ScmsPdu-Scoped ::= ScmsPdu (WITH COMPONENTS {
    ...,
    content (CONSTRAINED BY {
      --Pdu
    })
  })


  SignedCertificateRequest ::= SEQUENCE  {
    hashId      HashAlgorithm,
    signer      SignerIdentifier,
    signature   Signature
  }
 SignerSelf ::= SignerIdentifier(WITH COMPONENTS {
    self
  })
  SignerSingleCert ::= SignerIdentifier(WITH COMPONENTS {
    certificate (SequenceOfCertificate (SIZE (1)))
  })




END
```

---

## ScmsPdu.asn

- **ASN.1 模块名**：`ScmsPdu`
- **依赖模块：AcaEeInterfacePdu、CertManagementPdu、EcaEeInterfacePdu、AcaRaInterfacePdu、EeRaInterfacePdu、LaMaInterfacePdu、LaRaInterfacePdu、SecurityBaseDataTypes**

```asn1
ScmsPdu DEFINITIONS AUTOMATIC TAGS ::= BEGIN 

EXPORTS ALL;
 
IMPORTS 
AcaEeInterfacePdu FROM AcaEeInterfacePdu 
CertManagementPdu FROM CertManagementPdu
EcaEeInterfacePdu FROM EcaEeInterfacePdu
AcaRaInterfacePdu FROM AcaRaInterfacePdu 
EeRaInterfacePdu FROM EeRaInterfacePdu
LaMaInterfacePdu FROM LaMaInterfacePdu 
LaRaInterfacePdu FROM LaRaInterfacePdu
  Uint8 FROM SecurityBaseDataTypes
;


 ScmsPdu ::= SEQUENCE {
    version  Uint8 (1),
    content  CHOICE {
      aca-ee  AcaEeInterfacePdu,
      aca-la  AcaLaInterfacePdu,
      aca-ma  AcaMaInterfacePdu,
      aca-ra  AcaRaInterfacePdu,
      cert    CertManagementPdu,
      eca-ee  EcaEeInterfacePdu,
      ee-ma   EeMaInterfacePdu,
      ee-ra   EeRaInterfacePdu,
      la-ma   LaMaInterfacePdu,
      la-ra   LaRaInterfacePdu,
      ma-ra   MaRaInterfacePdu,
      ...
    }
  }
AcaLaInterfacePdu ::= NULL
AcaMaInterfacePdu ::= NULL
MaRaInterfacePdu ::= NULL
EeMaInterfacePdu ::= NULL


END
```

---

## ScmsSsp.asn

- **ASN.1 模块名**：`ScmsSsp`
- **依赖模块：SecurityBaseDataTypes**

```asn1
 ScmsSsp DEFINITIONS AUTOMATIC TAGS ::= BEGIN 

EXPORTS ScmsSsp ;
IMPORTS 
Uint8, SequenceOfAid, Uint16 FROM SecurityBaseDataTypes
;
ScmsSsp ::= CHOICE {
    root     RootCaSsp,
    ma       MaSsp,
    pg       PgSsp,
    ica      IcaSsp,
    eca      EcaSsp,
cra      CraSsp,
    dcm      DcmSsp,
    aca      AcaSsp,
    la       LaSsp,
    ra       RaSsp,
    ee       EeSsp,
    ...
  }
  RootCaSsp ::= SEQUENCE {
    version  Uint8 (1),
    ...
  }
 MaSsp ::= SEQUENCE {
    version        Uint8 (1),
    relevantAids  SequenceOfAid,
    ...
  }
  PgSsp ::= SEQUENCE {
    version  Uint8 (1),
    ...
  }
  IcaSsp ::= SEQUENCE {
    version  Uint8 (1),
    ...
  }
  EcaSsp ::= SEQUENCE {
    version  Uint8 (1),
    ...
  }
 AcaSsp ::= SEQUENCE {
    version  Uint8 (1),
    ...
  }
  CraSsp ::= SEQUENCE {
    version  Uint8 (1),
    ...
  }
  DcmSsp ::= SEQUENCE {
    version  Uint8 (1),
    ...
  }
  LaSsp ::= SEQUENCE {
    version  Uint8 (1),
    laId     Uint16,
    ...
  }
  RaSsp ::= SEQUENCE {
    version  Uint8 (1),
    ...
  }
  EeSsp ::= SEQUENCE {
    version  Uint8 (1),
    ...
  }


END
```

---

## AcaEeCertResponseCubkSpdu.asn

- **ASN.1 模块名**：`AcaEeCertResponseCubkSpdu`
- **依赖模块：SecurityBaseDataTypes、SecurityDataTypes、ScmsPdu、V2XSecData-Encrypted**

```asn1
AcaEeCertResponseCubkSpdu DEFINITIONS AUTOMATIC TAGS ::= BEGIN 

EXPORTS ALL;
IMPORTS Uint8,Time32 FROM SecurityBaseDataTypes
	Certificate,Aid FROM SecurityDataTypes
	V2XSecData FROM SecurityDataTypes
	ScmsPdu FROM ScmsPdu
V2XSecData-Encrypted,V2XSecData-Unsecured,V2XSecData-EncryptedSigned FROM V2XSecData-Encrypted
;


  AcaEeCertResponseCubkSpdu ::= V2XSecData-Encrypted {
    ScmsPdu-Scoped {
      AcaEeInterfacePdu (WITH COMPONENTS {
        acaEeCertResponse
      })
    }
  }
  AcaEeCertResponsePlainSpdu ::= V2XSecData-Unsecured {
    ScmsPdu-Scoped {
      AcaEeInterfacePdu (WITH COMPONENTS {
        acaEeCertResponse
      })
    }
  }
 AcaEeCertResponsePrivateSpdu ::= V2XSecData-EncryptedSigned {
    ScmsPdu-Scoped {
      AcaEeInterfacePdu (WITH COMPONENTS {
        acaEeCertResponse
      })
    }, 
    SecurityMgmtAid
  }
 CertificateChainSpdu ::= V2XSecData-Unsecured {
    ScmsPdu-Scoped {
      CertManagementPdu (WITH COMPONENTS {
        certificateChain
      })
    }
  }
  CompositeCrlSpdu ::= V2XSecData-Unsecured {
    ScmsPdu-Scoped {
      CertManagementPdu (WITH COMPONENTS {
        compositeCrl
      })
    }
  }
  EcaEeCertResponseSpdu ::= V2XSecData-Signed {
    ScmsPdu-Scoped {
      EcaEeInterfacePdu (WITH COMPONENTS {
        ecaEeCertResponse
      })
    }, 
    SecurityMgmtAid
  }
 EeEcaCertRequestSpdu ::= V2XSecData-SignedCertRequest {
    ScmsPdu-Scoped {
      EcaEeInterfacePdu (WITH COMPONENTS {
        eeEcaCertRequest
      })
    }, 
    SignerSelf
  }
 EeRaCertRequestPlainSpdu ::= V2XSecData-Unsecured {
    ScmsPdu-Scoped {
      EeRaInterfacePdu (WITH COMPONENTS {
        eeRaCertRequest
      })
    }
  }
  EeRaCertRequestSpdu ::= V2XSecData-SignedEncryptedCertRequest {
    ScmsPdu-Scoped {
      EeRaInterfacePdu (WITH COMPONENTS {
        eeRaCertRequest
      })
    }, 
    SignerSingleCert
  }
 EeRaDownloadRequestPlainSpdu ::= V2XSecData-Unsecured {
    ScmsPdu-Scoped {
      EeRaInterfacePdu (WITH COMPONENTS {
        eeRaDownloadRequest
      })
    }
  }
 EeRaDownloadRequestSpdu ::= V2XSecData-SignedEncrypted {
    ScmsPdu-Scoped {
      EeRaInterfacePdu (WITH COMPONENTS {
        eeRaDownloadRequest
      })
    }, 
    SecurityMgmtAid
  }
EeRaSuccessorEnrollmentCertRequestSpdu ::=
    V2XSecData-SignedEncryptedCertRequest { ScmsPdu-Scoped {
      EeRaInterfacePdu (WITH COMPONENTS {
        eeRaSuccessorEnrollmentCertRequest
      })
    }, 
    SignerSingleCert
  }
RaEeCertAckSpdu ::= V2XSecData-Signed {
    ScmsPdu-Scoped {
      EeRaInterfacePdu (WITH COMPONENTS {
        raEeCertAck
      })
    }, 
    SecurityMgmtAid
  }
 RaEeCertInfoSpdu ::= V2XSecData-Unsecured {
    ScmsPdu-Scoped {
      EeRaInterfacePdu (WITH COMPONENTS {
        raEeCertInfo
      })
    }
  }
 RaEeEnrollmentCertAckSpdu ::= V2XSecData-Signed {
    ScmsPdu-Scoped {
      EeRaInterfacePdu (WITH COMPONENTS {
        raEeCertInfo
      })
    }, 
    SecurityMgmtAid
  }
SecurityMgmtAid ::= Aid (3627)

END
```

---

## AcaEeInterfacePdu.asn

- **ASN.1 模块名**：`AcaEeInterfacePdu`
- **依赖模块：SecurityBaseDataTypes、SecurityDataTypes**

```asn1
 AcaEeInterfacePdu DEFINITIONS AUTOMATIC TAGS ::= BEGIN 

EXPORTS AcaEeInterfacePdu;
IMPORTS Uint8,Time32 FROM SecurityBaseDataTypes
Certificate FROM SecurityDataTypes
;
 AcaEeInterfacePdu::= CHOICE {
    acaEeCertResponse  AcaEeCertResponse,
    ...
  }
  AcaEeCertResponse ::= SEQUENCE {
    version         Uint8 (1),
    generationTime  Time32,
    certificate     Certificate,
    privateKeyInfo  OCTET STRING (SIZE (32)) OPTIONAL,
    ...
  }
END
```

---

## AcaRaInterfacePdu.asn

- **ASN.1 模块名**：`AcaRaInterfacePdu`
- **依赖模块：SecurityBaseDataTypes、SecurityDataTypes、ScmsPdu、V2XSecData-Encrypted、AcaEeCertResponseCubkSpdu、LaRaInterfacePdu**

```asn1
AcaRaInterfacePdu DEFINITIONS AUTOMATIC TAGS ::= BEGIN 

EXPORTS ALL;
IMPORTS Uint8,Time32 FROM SecurityBaseDataTypes
	Certificate FROM SecurityDataTypes
	V2XSecData FROM SecurityDataTypes
	ScmsPdu FROM ScmsPdu
V2XSecData-SymmEncryptedSingleRecipient FROM V2XSecData-Encrypted
AcaEeCertResponsePlainSpdu,AcaEeCertResponsePrivateSpdu FROM AcaEeCertResponseCubkSpdu
AcaEeCertResponseCubkSpdu FROM AcaEeCertResponseCubkSpdu 
EncryptedIndividualLV FROM LaRaInterfacePdu

;
 
AcaRaInterfacePdu ::= CHOICE {
    raAcaCertRequest   RaAcaCertRequest,
    acaRaCertResponse  AcaRaCertResponse,
    ...
  }
RaAcaCertRequest ::= SEQUENCE {
    version         Uint8 (1),
    generationTime  Time32,
    flags           RaAcaCertRequestFlags,
    linkageInfo     EncryptedIndividualLV OPTIONAL,
    certEncKey      PublicEncryptionKey OPTIONAL,
    tbsCert         ToBeSignedCertificate (WITH COMPONENTS { 
      ...,
      cracaId ('000000'H),
      crlSeries (0),
      appPermissions PRESENT,
      certIssuePermissions ABSENT,
      certRequestPermissions ABSENT 
    }),
    ...
  }
  RaAcaCertRequestFlags ::= BIT STRING {butterflyExplicit (0), useCubk (1)} (SIZE (8))
  AcaRaCertResponse ::= SEQUENCE {
    version         Uint8 (1),
    generationTime  Time32,
    requestHash     HashedId8,
    acaResponse     AcaResponse,
    ...
  }
  AcaResponse ::= CHOICE {
    plain    AcaEeCertResponsePlainSpdu,
    private  AcaEeCertResponsePrivateSpdu,
    cubk     AcaEeCertResponseCubkSpdu,
    ...
  }
  RaAcaCertRequestSpdu ::= V2XSecData-SignedCertRequest {
    ScmsPdu-Scoped {
      AcaRaInterfacePdu (WITH COMPONENTS {
        raAcaCertRequest
      })
    }, 
    SignerSingleCert
  }
  AcaRaCertResponseSpdu ::= V2XSecData-Signed {
    ScmsPdu-Scoped {
      AcaRaInterfacePdu (WITH COMPONENTS {
        acaRaCertResponse
      })
    }, 
    SecurityMgmtAid
  }

END
```

---

## EcaEeInterfacePdu.asn

- **ASN.1 模块名**：`EcaEeInterfacePdu`
- **依赖模块：SecurityBaseDataTypes、SecurityDataTypes**

```asn1
 EcaEeInterfacePdu DEFINITIONS AUTOMATIC TAGS ::= BEGIN 

EXPORTS EcaEeInterfacePdu;
IMPORTS Uint8,Time32 FROM SecurityBaseDataTypes
Certificate FROM SecurityDataTypes

;
 
  EcaEeInterfacePdu::= CHOICE {
    eeEcaCertRequest   EeEcaCertRequest,
    ecaEeCertResponse  EcaEeCertResponse,
    ...
  }EeEcaCertRequest ::= SEQUENCE {
    version         Uint8 (1),
    generationTime  Time32,
    type            CertificateType,
    tbsCert         ToBeSignedCertificate (WITH COMPONENTS {
      ...,
      id (WITH COMPONENTS {
        ..., 
        linkageData ABSENT
      }),
      cracaId ('000000'H),
      crlSeries (0),
      appPermissions ABSENT,
      certIssuePermissions ABSENT,
      certRequestPermissions PRESENT,
      verifyKeyIndicator  (WITH COMPONENTS {
        verificationKey
      })
    }),
    canonicalId     IA5String OPTIONAL,
    ...
  }

 EcaEeCertResponse ::= SEQUENCE {
    version         Uint8 (1),
    requestHash     HashedId8,
    ecaCertChain    SequenceOfCertificate,
    certificate     Certificate,
    privateKeyInfo  OCTET STRING (SIZE(32)) OPTIONAL,
    ...
  }

END
```

---

## EeRaInterfacePdu.asn

- **ASN.1 模块名**：`EeRaInterfacePdu`
- **依赖模块：SecurityBaseDataTypes、SecurityDataTypes、AcaEeCertResponseCubkSpdu**

```asn1
 EeRaInterfacePdu DEFINITIONS AUTOMATIC TAGS ::= BEGIN 

EXPORTS EeRaInterfacePdu;
IMPORTS Uint8,Time32 FROM SecurityBaseDataTypes
Certificate FROM SecurityDataTypes
EeEcaCertRequestSpdu FROM AcaEeCertResponseCubkSpdu
;
 
 EeRaInterfacePdu ::= CHOICE {
    eeRaCertRequest                     EeRaCertRequest,
    raEeCertAck                         RaEeCertAck,
    raEeCertInfo                        RaEeCertInfo,
    eeRaDownloadRequest                 EeRaDownloadRequest,
    eeRaSuccessorEnrollmentCertRequest  EeEcaCertRequestSpdu,
    ...
  }
EeRaCertRequest ::= SEQUENCE {
    version           Uint8 (1),
    generationTime    Time32,
    type              CertificateType,
    tbsCert           ToBeSignedCertificate (WITH COMPONENTS { 
      ...,
      cracaId ('000000'H),
      crlSeries (0),
      appPermissions PRESENT,
      certIssuePermissions ABSENT,
      certRequestPermissions ABSENT,
      verifyKeyIndicator (WITH COMPONENTS {
        verificationKey
      })
    }),
    additionalParams  AdditionalParams OPTIONAL,
    ...,
aaaAuthorizationInfoList SequenceOfAaaAuthorizationInfo OPTIONAL
}
 AdditionalParams ::= CHOICE {
    original        ButterflyParamsOriginal,
    unified         ButterflyExpansion,  
    compactUnified  ButterflyExpansion,
    encryptionKey   PublicEncryptionKey,
    ...
  }
 ButterflyParamsOriginal ::= SEQUENCE {
    signingExpansion     ButterflyExpansion,
    encryptionKey        PublicEncryptionKey,
    encryptionExpansion  ButterflyExpansion
  }
ButterflyExpansion ::= CHOICE {
    aes128      OCTET STRING (SIZE(16)),
    ...,
    sm4         OCTET STRING(SIZE(16))
  }
  RaEeCertInfo ::= SEQUENCE {
    version         Uint8 (1),
    generationTime  Time32,
    currentI        IValue,
    requestHash     HashedId8,
    nextDlTime      Time32,
    ...
  }
 EeRaDownloadRequest ::= SEQUENCE {
    generationTime  Time32,
    filename        UTF8String (SIZE (0..255)),
    ...
  }
 AaaAuthorizationInfo ::= SEQUENCE {
    aaaAuthorizationInfo	Opaque,
    aaaUrl			UTF8String (SIZE (0..511)) OPTIONAL
  }
SequenceOfAaaAuthorizationInfo ::= SEQUENCE OF AaaAuthorizationInfo
  RaEeCertAck ::= SEQUENCE {
    version         Uint8 (1),
    generationTime  Time32,
    requestHash     HashedId8,
    nextDlTime      Time32,
    ...
  }


END
```

---

## LaMaInterfacePdu.asn

- **ASN.1 模块名**：`LaMaInterfacePdu`
- **依赖模块：SecurityBaseDataTypes、V2XSecData-Encrypted**

```asn1
 LaMaInterfacePdu DEFINITIONS AUTOMATIC TAGS ::= BEGIN 

EXPORTS LaMaInterfacePdu ;
IMPORTS 
IValue, Uint32, LinkageValue, HashedId8, LaId, Uint32, LinkageSeed FROM SecurityBaseDataTypes
V2XSecData-SignedEncrypted FROM V2XSecData-Encrypted

;
 


LaMaInterfacePdu ::= CHOICE{
maLaLinkageSeedRequest MaLaLinkageSeedRequest,
laMaLinkageSeedResponse LaMaLinkageSeedResponse,
...
}
MaLaLinkageSeedRequest ::= SEQUENCE{
maId     	         OCTET STRING (SIZE(0..255)),
individualLVs       SEQUENCE (SIZE(0..MAX)) OF IndividualLV,
groupLVs            SEQUENCE (SIZE(0..MAX)) OF GroupLV
 }
IndividualLV ::= SEQUENCE {
    iValue     IValue,
    lv         LinkageValue
}
GroupLV::= SEQUENCE {
    iValue     IValue,
    j          Uint32,
    lv         LinkageValue 
}
LaMaLinkageSeedResponse ::= SEQUENCE{
    requestHash            HashedId8,
    laId                   LaId,
    individualLSInfos      SEQUENCE(SIZE(0..MAX)) OF IndividualLSInfo,
    groupLSInfos           SEQUENCE(SIZE(0..MAX)) OF GroupLSInfo
}
IndividualLSInfo ::= SEQUENCE {
      i    IValue,
      lv   LinkageValue,
      ls   LinkageSeed
}
GroupLSInfo ::= SEQUENCE {
      i     IValue, 
      j     Uint32,
      lv    LinkageValue,
      ls    LinkageSeed 
}
MaLaLinkageSeedRequestSpdu ::= V2XSecData-SignedEncrypted {
 ScmsPdu-Scoped {
      LaMaInterfacePdu (WITH COMPONENTS {
        	maLaLinkageSeedRequest
      })
    }, 
    SecurityMgmtAid
}
LaMaLinkageSeedResponseSpdu ::= V2XSecData-SignedEncrypted {
 ScmsPdu-Scoped {
      LaMaInterfacePdu (WITH COMPONENTS {
        	laMaLinkageSeedResponse
      })
    }, 
    SecurityMgmtAid
  }

END
```

---

## LaRaInterfacePdu.asn

- **ASN.1 模块名**：`LaRaInterfacePdu`
- **依赖模块：SecurityDataTypes、V2XSecData-Encrypted、SecurityBaseDataTypes**

```asn1
LaRaInterfacePdu DEFINITIONS AUTOMATIC TAGS ::= BEGIN 

EXPORTS ALL;
IMPORTS 
    RecipientInfo FROM SecurityDataTypes
    ScmsPdu-Scoped FROM V2XSecData-Encrypted
    Uint8, Uint32, Uint16, HostnameLaId FROM SecurityBaseDataTypes
V2XSecData-SignedEncrypted FROM V2XSecData-Encrypted
;
LaRaInterfacePdu ::= CHOICE{
praLaLinkageValueRequest PraLaLinkageValueRequest,
laPraLinkageValueResponse LaPraLinkageValueResponse,
...
}
PraLaLinkageValueRequest ::= CHOICE{
   individualLVRequest  PraLaIndividualLinkageValueRequest,
   groupLVRequest PraLaGroupLinkageValueRequest
}
PraLaIndividualLinkageValueRequest ::= SEQUENCE {
header              PraLaRequestHeader,
jMax                Uint8,
numberOfFreshInd    Uint32 OPTIONAL,
continuationsInd    SEQUENCE OF LinkageChainId OPTIONAL
}
LinkageChainId ::= EncryptedData (WITH COMPONENTS {
    recipients (SIZE (1) INTERSECTION (WITH COMPONENT (
        RecipientInfo (WITH COMPONENTS {
            certRecipInfo
    })))),
    ciphertext
  }  
)
PraLaGroupLinkageValueRequest ::= SEQUENCE {
    header              PraLaRequestHeader,
    jMax                Uint32,
    groupIdentifier     OCTET STRING (SIZE(4)) 
}
PraLaRequestHeader ::= SEQUENCE{
iMin               Uint16,
iMax               Uint16,
praId	        Hostname,
pcaId              Hostname
}
LaPraLinkageValueResponse ::= SEQUENCE {
    iMin                 Uint16,
    iMax                 Uint16,
    individualLVs       SEQUENCE (SIZE (0..MAX)) OF  IndividualLVResponseLinkageChain,
    groupLVs                 SEQUENCE (SIZE (0..MAX)) OF GroupLVResponseLinkageChain
}
IndividualLVResponseLinkageChain ::= SEQUENCE {
    jMax                Uint8,
values              SEQUENCE OF 
				  SEQUENCE OF	EncryptedIndividualLV,
    linkageChainId     LinkageChainId
}
GroupLVResponseLinkageChain ::= SEQUENCE {
    jMax                Uint32,
values             SEQUENCE OF 
				SEQUENCE OF EncryptedGroupLV,         
    groupIdentifier      OCTET STRING (SIZE(4))
}
EncryptedIndividualLV ::= SEQUENCE {
    version         Uint8(1),
    laId            LaId,            
    enc-lv         EncryptedData (WITH COMPONENTS {
      recipients (SIZE (1) INTERSECTION (WITH COMPONENT (           
        RecipientInfo (WITH COMPONENTS {        
            certRecipInfo 
      })))),
	ciphertext
    })
}
EncryptedGroupLV ::= SEQUENCE {
    version         Uint8(1),
    laId            LaId,                                
    enc-lv         EncryptedData (WITH COMPONENTS {
      recipients (SIZE (1) INTERSECTION (WITH COMPONENT (           
        RecipientInfo (WITH COMPONENTS {        
            certRecipInfo     
      })))),
	ciphertext
    })
}
PraLaIndividualLinkageValueRequestSpdu ::= V2XSecData-SignedEncrypted {
    ScmsPdu-Scoped {
      LaRaInterfacePdu (WITH COMPONENTS{
        praLaLinkageValueRequest (WITH COMPONENTS{
		individualLVRequest
	})
     })
    }, 
    SecurityMgmtAid
  }
PraLaGroupLinkageValueRequestSpdu ::= V2XSecData-SignedEncrypted {
    ScmsPdu-Scoped {
      LaRaInterfacePdu  (WITH COMPONENTS {
        praLaLinkageValueRequest (WITH COMPONENTS {
		groupLVRequest  
	})
      })
    }, 
    SecurityMgmtAid
  }
LaPraLinkageValueResponseSpdu ::= V2XSecData-SignedEncrypted {
    ScmsPdu-Scoped {
      LaRaInterfacePdu  (WITH COMPONENTS {
        laPraLinkageValueResponse
      })
    }, 
    SecurityMgmtAid
 }




END
```

---

## CertManagementPdu.asn

- **ASN.1 模块名**：`CertManagementPdu`
- **依赖模块：SecurityBaseDataTypes、SecuredCrl、SecurityDataTypes、AcaEeCertResponseCubkSpdu**

```asn1
 CertManagementPdu DEFINITIONS AUTOMATIC TAGS ::= BEGIN 

EXPORTS CertManagementPdu;
IMPORTS Uint8,Time32 FROM SecurityBaseDataTypes
SecuredCrl FROM SecuredCrl 
Certificate FROM SecurityDataTypes
AcaEeCertResponseCubkSpdu,CtlSignatureSpdu FROM AcaEeCertResponseCubkSpdu
;

 CertManagementPdu ::= CHOICE {
    compositeCrl                    CompositeCrl,
    certificateChain                CertificateChain,
    ...
  }
CompositeCrl ::= SEQUENCE {
    crl      SEQUENCE SIZE(0..MAX) OF SecuredCrl,
    ...
  }
  CertificateChain ::= SEQUENCE {
    others     SEQUENCE SIZE(0..MAX) OF Certificate,
    ...
  }
END
```

---

## CTL.asn

- **ASN.1 模块名**：`CTL`
- **依赖模块：SecurityBaseDataTypes、SecurityDataTypes**

```asn1
CTL DEFINITIONS AUTOMATIC TAGS ::= BEGIN 

EXPORTS ALL;
IMPORTS   Uint8,HashedId,Time32 FROM SecurityBaseDataTypes
Certificate, IssuerIdentifier,SequenceOfCertificate, Signature
 FROM SecurityDataTypes
;
  RootCtl ::= SEQUENCE {
    toBeSignedRootCtl	ToBeSignedRootCtl,
    signature		Signature
  }
ToBeSignedRootCtl ::= SEQUENCE  {
    version          	Uint8 (1),
    issuer           	IssuerIdentifier,
    series           	Uint16,
    issueDate        	Time32,
    nextRootCtl	Time32,
pkiCtlInfoList	SequenceOfPkiCtlInfo
}
PkiCtlInfo ::= SEQUENCE  {
rootCertificate	Certificate,
caListUrl		UTF8String (SIZE (0..511)) OPTIONAL
}
SequenceOfPkiCtlInfo ::= SEQUENCE OF PkiCtlInfo
 PkiCtl ::= SEQUENCE {
    toBeSignedPkiCtl		ToBeSignedPkiCtl,
    signature		Signature
  }
ToBeSignedPkiCtl ::= SEQUENCE  {
    version          	Uint8 (1),
    issuer           	IssuerIdentifier,
    series           	Uint16,
    issueDate        	Time32,
    nextPkiCtl		Time32,
pkiCertInfoList	SequenceOfPkiCertInfo
}
PkiCertInfo ::= SEQUENCE  {
certificate	SequenceOfCertificate,
crlUrl		UTF8String (SIZE (0..511)) OPTIONAL,
maUrl		UTF8String (SIZE (0..511)) OPTIONAL
}
SequenceOfPkiCertInfo ::= SEQUENCE OF PkiCertInfo

END
```

---

## CRLBaseTypes.asn

- **ASN.1 模块名**：`CRLBaseTypes`
- **依赖模块：SecurityBaseDataTypes**

```asn1
CRLBaseTypes DEFINITIONS AUTOMATIC TAGS ::= BEGIN 

EXPORTS ALL;
 
IMPORTS 
  CrlSeries,
  GeographicRegion,
  HashedId8,
  HashedId10,
  IValue,
  LaId,
  LinkageSeed,
  Opaque,
  Aid,
  Signature,
  Time32,
  Uint3,
  Uint8,
  Uint16, 
  Uint32,
  ValidityPeriod
FROM SecurityBaseDataTypes

;
--
--
--  CRL contents
--
--

CrlContents ::= SEQUENCE  {
    version            Uint8 (1),
    crlSeries          CrlSeries,
    crlCraca            HashedId8,
    issueDate          Time32,   
    nextCrl            Time32,  
    priorityInfo       CrlPriorityInfo,
    typeSpecific       CHOICE  {
      fullHashCrl          ToBeSignedHashIdCrl,            
      deltaHashCrl         ToBeSignedHashIdCrl,            
      fullLinkedCrl        ToBeSignedLinkageValueCrl,
      deltaLinkedCrl       ToBeSignedLinkageValueCrl,
      ...
    }
}    

CrlPriorityInfo ::= SEQUENCE  {  
    priority          Uint8 OPTIONAL,
    ...
}


ToBeSignedHashIdCrl ::= SEQUENCE  {  
    crlSerial         Uint32,
    entries           SequenceOfHashBasedRevocationInfo,
    ...
}

HashBasedRevocationInfo ::= SEQUENCE  {
    id         HashedId10,
    expiry     Time32,
    ...
}

SequenceOfHashBasedRevocationInfo ::= 
    SEQUENCE OF HashBasedRevocationInfo


ToBeSignedLinkageValueCrl ::= SEQUENCE  {  
    iRev               IValue,
    indexWithinI       Uint8,
    individual         SequenceOfJMaxGroup OPTIONAL,
    groups             SequenceOfGroupCrlEntry OPTIONAL,
    ...,
   groupsSingleSeed   SequenceOfGroupSingleSeedCrlEntry OPTIONAL
} 
(WITH COMPONENTS {..., individual PRESENT} |
 WITH COMPONENTS {..., groups PRESENT})


JMaxGroup ::= SEQUENCE  {
    jmax              Uint8,
    contents          SequenceOfLAGroup,
    ...
}


SequenceOfJMaxGroup ::= SEQUENCE OF JMaxGroup

LAGroup ::= SEQUENCE  {
    la1Id            LaId,
    la2Id            LaId,
    contents         SequenceOfIMaxGroup,
    ... 
}

SequenceOfLAGroup ::= SEQUENCE OF LAGroup

IMaxGroup ::= SEQUENCE {
    iMax              Uint16,
    contents          SequenceOfIndividualRevocation,
    ...,
    singleSeeds        SequenceOfLinkageSeed
}

SequenceOfIMaxGroup ::= SEQUENCE OF IMaxGroup

IndividualRevocation ::= SEQUENCE  { 
    linkageSeed1    LinkageSeed,
    linkageSeed2     LinkageSeed,
   
    ...
}

SequenceOfIndividualRevocation ::= SEQUENCE OF IndividualRevocation
LinkageSeed ::= OCTET STRING (SIZE(16))
SequenceOfLinkageSeed ::= SEQUENCE OF LinkageSeed

GroupCrlEntry ::= SEQUENCE  {
    iMax             Uint16,
    la1Id            LaId,
    linkageSeed1     LinkageSeed,
    la2Id            LaId,
    linkageSeed2     LinkageSeed,
    ...
}

SequenceOfGroupCrlEntry ::= SEQUENCE OF GroupCrlEntry

---GroupSingleSeedCrlEntry
GroupSingleSeedCrlEntry ::= SEQUENCE {
   iMax     Uint16,
   laid     LaId,
   linkageSeed   LinkageSeed
}

SequenceOfGroupSingleSeedCrlEntry ::= SEQUENCE OF GroupSingleSeedCrlEntry


END
```

---

## CRLSecurityPacage.asn

- **ASN.1 模块名**：`SecuredCrl`
- **依赖模块：SecurityDataTypes**

```asn1
SecuredCrl DEFINITIONS AUTOMATIC TAGS ::= BEGIN 

EXPORTS ALL;
 
IMPORTS 

  V2XSecData,
  Opaque,
  Aid,
  CrlContents FROM SecurityDataTypes
;

CrlAid ::= Aid(3628)

SecuredCrl ::= V2XSecData (WITH COMPONENTS {..., 
  content (WITH COMPONENTS {
    signedData  (WITH COMPONENTS {..., 
      tbsData (WITH COMPONENTS {
        payload (WITH COMPONENTS {..., 
          data (WITH COMPONENTS {...,
             content (WITH COMPONENTS {
                unsecuredData (CONTAINING CrlContents)
            })
          })
        }),
        headerInfo (WITH COMPONENTS {..., 
          aid (CrlAid),
          generationTime ABSENT,
          expiryTime ABSENT,
          generationLocation ABSENT,
          p2pcdLearningRequest ABSENT,
          missingCrlIdentifier ABSENT,
          encryptionKey ABSENT
        })
      })
    })
  })
})

END
```

---

## CRLSSP.asn

- **ASN.1 模块名**：`CrlSsp`
- **依赖模块：SecurityBaseDataTypes**

```asn1
CrlSsp DEFINITIONS AUTOMATIC TAGS ::= BEGIN 

EXPORTS ALL;
 
IMPORTS 
  CrlSeries,
  Uint8 FROM SecurityBaseDataTypes
;


CrlSsp::= SEQUENCE  {
    version         Uint8(1),
    associatedCraca CracaType,
    crls            PermissibleCrls,
    ...
}

CracaType ::= ENUMERATED {isCraca, issuerIsCraca}
      
PermissibleCrls ::= SEQUENCE OF CrlSeries


END
```

---

## XDJA-EC-PC.asn

- **ASN.1 模块名**：`CCSA`
- **依赖模块：AcaEeCertResponseCubkSpdu、V2XSecData-Encrypted**

```asn1
CCSA DEFINITIONS AUTOMATIC TAGS ::= BEGIN 
 
EXPORTS ALL;
IMPORTS
    EeEcaCertRequestSpdu FROM AcaEeCertResponseCubkSpdu
    RaEeCertAckSpdu FROM AcaEeCertResponseCubkSpdu
    SecurityMgmtAid FROM AcaEeCertResponseCubkSpdu
    V2XSecData-SignedEncrypted FROM V2XSecData-Encrypted
    V2XSecData-Encrypted FROM V2XSecData-Encrypted
;

--EC??
EeEcaCertRequestEncSpdu ::= V2XSecData-SignedEncrypted {
    EeEcaCertRequestSpdu, 
    SecurityMgmtAid
}

--EC????
EcaEeCertResponseEncSpdu ::= V2XSecData-Encrypted {
    EeEcaCertRequestSpdu
}

--PC????
RaEeCertAckEncSpdu ::= V2XSecData-Encrypted {
    RaEeCertAckSpdu
}

END
```
