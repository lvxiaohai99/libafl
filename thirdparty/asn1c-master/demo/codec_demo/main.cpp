/**
 * asn1 codec demo：覆盖 BSM / RSM / SPAT / MAP / RSI 的 UPER、XER 编解码。
 *
 * 重点验证：
 *   - SEQUENCE OF 列表（RSM participants、SPAT phases、MAP nodes、RSI rtes/rtss）
 *   - OPTIONAL（有/无两种）
 *   - CHOICE（MessageFrame、PositionOffsetLL、TimeChangeDetails、Description）
 *   - BIT STRING（IntersectionStatusObject、ReferenceLanes）
 *   - 从 .uper / .xml 文件解析回 MessageFrame
 *
 * 用法：
 *   ./asn1_codec_demo                 # 默认：五类消息全量 roundtrip
 *   ./asn1_codec_demo encode-sample -o samples
 *   ./asn1_codec_demo decode --uper samples/rsi.uper
 *   ./asn1_codec_demo decode --xer  samples/rsi.xml
 */

#include "MessageFrame.h"

#include "afl/asn1/Asn1Cpp.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

AFL_DECLARE_ASN1_TYPE(MessageFrame)
AFL_DECLARE_ASN1_TYPE(RTEList)
AFL_DECLARE_ASN1_TYPE(RTSList)

using afl::asn1::Asn1List;
using afl::asn1::B_UPER;
using afl::asn1::B_XER;
using afl::asn1::BitString;
using afl::asn1::MessageFramePtr;
using afl::asn1::fragment;

namespace {

void setOctet(OCTET_STRING_t& os, const char* bytes, size_t n)
{
    free(os.buf);
    os.buf = static_cast<uint8_t*>(calloc(n + 1, 1));
    os.size = n;
    if (os.buf && n) {
        std::memcpy(os.buf, bytes, n);
    }
}

void setIa5(OCTET_STRING_t& os, const char* text)
{
    setOctet(os, text, std::strlen(text));
}

void fillPos3D(Position3D_t& pos, long lat, long lon, bool withElevation)
{
    pos.lat = lat;
    pos.Long = lon;
    if (withElevation) {
        fragment(pos.elevation);
        *pos.elevation = 100; /* 示意：厘米级偏移编码 */
    }
}

void fillOffsetLL24(PositionOffsetLLV_t& pov, long dLon, long dLat, bool withVert)
{
    pov.offsetLL.present = PositionOffsetLL_PR_position_LL1;
    fragment(pov.offsetLL.position_LL1);
    pov.offsetLL.position_LL1->lon = dLon;
    pov.offsetLL.position_LL1->lat = dLat;
    if (withVert) {
        fragment(pov.offsetV);
        pov.offsetV->present = VerticalOffset_PR_offset1;
        /* offset1 为 VertOffset-B07 等，直接赋整数成员名见生成头 */
        pov.offsetV->offset1 = 5;
    }
}

/* ---------- BSM：基础 SEQUENCE + 少量 OPTIONAL ---------- */
void fillBsm(MessageFramePtr& mf)
{
    mf->present = MessageFrame_PR_bsmFrame;
    fragment(mf->bsmFrame);
    BasicSafetyMessage_t* bsm = mf->bsmFrame;

    bsm->msgCnt = 7;
    setOctet(bsm->id, "DEMO0001", 8);
    bsm->secMark = 12345;
    fillPos3D(bsm->pos, 310000000, 1210000000, true); /* elevation OPTIONAL 有 */

    bsm->transmission = TransmissionState_forwardGears;
    bsm->speed = 200;
    bsm->heading = 9000;
    bsm->accelSet.Long = 0;
    bsm->accelSet.lat = 0;
    bsm->accelSet.vert = 0;
    bsm->accelSet.yaw = 0;
    bsm->size.width = 180;
    bsm->size.length = 450;
    bsm->vehicleClass.classification = 10;
    /* angle / safetyExt 等 OPTIONAL 故意不填 → 测「缺省」 */
}

/* ---------- RSM：列表 + 多 OPTIONAL（有/无混用）+ CHOICE 偏移 ---------- */
void fillRsm(MessageFramePtr& mf)
{
    mf->present = MessageFrame_PR_rsmFrame;
    fragment(mf->rsmFrame);
    RoadsideSafetyMessage_t* rsm = mf->rsmFrame;

    rsm->msgCnt = 3;
    setOctet(rsm->id, "RSU00001", 8);
    fillPos3D(rsm->refPos, 311000000, 1211000000, false); /* elevation 故意不填 */

    Asn1List<ParticipantList_t> plist(rsm->participants);

    /* 参与者 0：带 OPTIONAL id / transmission / accelSet */
    {
        ParticipantData_t* p = 0;
        plist.push(p);
        p = plist.back();
        p->ptcType = ParticipantType_motor;
        p->ptcId = 1;
        p->source = SourceType_v2x;
        fragment(p->id);
        setOctet(*p->id, "VEH00001", 8);
        p->secMark = 1000;
        fillOffsetLL24(p->pos, 10, -20, true);
        p->posConfidence.pos = PositionConfidence_a1m;
        fragment(p->posConfidence.elevation);
        *p->posConfidence.elevation = ElevationConfidence_elev_000_50;
        fragment(p->transmission);
        *p->transmission = TransmissionState_forwardGears;
        p->speed = 150;
        p->heading = 1800;
        fragment(p->accelSet);
        p->accelSet->Long = 1;
        p->accelSet->lat = 2;
        p->accelSet->vert = 0;
        p->accelSet->yaw = 0;
        p->size.width = 170;
        p->size.length = 400;
        fragment(p->vehicleClass);
        p->vehicleClass->classification = 20;
    }

    /* 参与者 1：OPTIONAL 尽量不填，只保留必填 */
    {
        ParticipantData_t* p = 0;
        plist.push(p);
        p = plist.back();
        p->ptcType = ParticipantType_pedestrian;
        p->ptcId = 2;
        p->source = SourceType_video;
        p->secMark = 2000;
        fillOffsetLL24(p->pos, 0, 0, false);
        p->posConfidence.pos = PositionConfidence_a5m;
        p->speed = 20;
        p->heading = 0;
        p->size.width = 50;
        p->size.length = 50;
    }
}

/* ---------- SPAT：嵌套列表 + BIT STRING + CHOICE timing ---------- */
void fillSpat(MessageFramePtr& mf)
{
    mf->present = MessageFrame_PR_spatFrame;
    fragment(mf->spatFrame);
    SPAT_t* spat = mf->spatFrame;

    spat->msgCnt = 9;
    fragment(spat->moy);
    *spat->moy = 100000;
    fragment(spat->timeStamp);
    *spat->timeStamp = 5000;
    fragment(spat->name);
    setIa5(*spat->name, "demo-spat");

    Asn1List<IntersectionStateList_t> isects(spat->intersections);
    IntersectionState_t* is = 0;
    isects.push(is);
    is = isects.back();

    is->intersectionId.id = 101;
    fragment(is->intersectionId.region);
    *is->intersectionId.region = 1;

    /* BIT STRING (SIZE 16) */
    BitString status(is->status, 16);
    status.set(IntersectionStatusObject_fixedTimeOperation);
    status.set(IntersectionStatusObject_recentMAPmessageUpdate);

    fragment(is->moy);
    *is->moy = 100001;

    Asn1List<PhaseList_t> phases(is->phases);
    Phase_t* ph = 0;
    phases.push(ph);
    ph = phases.back();
    ph->id = 1;

    Asn1List<PhaseStateList_t> states(ph->phaseStates);
    /* state0：有 timing OPTIONAL */
    {
        PhaseState_t* st = 0;
        states.push(st);
        st = states.back();
        st->light = LightState_protected_green;
        fragment(st->timing);
        st->timing->present = TimeChangeDetails_PR_counting;
        fragment(st->timing->counting);
        st->timing->counting->startTime = 100;
        st->timing->counting->likelyEndTime = 200;
        fragment(st->timing->counting->minEndTime);
        *st->timing->counting->minEndTime = 180;
    }
    /* state1：无 timing */
    {
        PhaseState_t* st = 0;
        states.push(st);
        st = states.back();
        st->light = LightState_red;
    }
}

/* ---------- MAP：NodeList + name/region OPTIONAL 混用（不填 inLinks 以控制复杂度） ---------- */
void fillMap(MessageFramePtr& mf)
{
    mf->present = MessageFrame_PR_mapFrame;
    fragment(mf->mapFrame);
    MapData_t* map = mf->mapFrame;

    map->msgCnt = 2;
    fragment(map->timeStamp);
    *map->timeStamp = 200000;

    Asn1List<NodeList_t> nodes(map->nodes);

    /* node0：有 name、有 region */
    {
        Node_t* n = 0;
        nodes.push(n);
        n = nodes.back();
        fragment(n->name);
        setIa5(*n->name, "node-A");
        fragment(n->id.region);
        *n->id.region = 7;
        n->id.id = 1;
        fillPos3D(n->refPos, 312000000, 1212000000, false);
    }
    /* node1：无 name、无 region */
    {
        Node_t* n = 0;
        nodes.push(n);
        n = nodes.back();
        n->id.id = 2;
        fillPos3D(n->refPos, 312000100, 1212000100, true);
    }
}

/* ---------- RSI：RTE/RTS 双列表 + Description CHOICE + ReferenceLanes BIT STRING ---------- */
void fillRsi(MessageFramePtr& mf)
{
    mf->present = MessageFrame_PR_rsiFrame;
    fragment(mf->rsiFrame);
    RoadSideInformation_t* rsi = mf->rsiFrame;

    rsi->msgCnt = 5;
    fragment(rsi->moy);
    *rsi->moy = 300000;
    setOctet(rsi->id, "RSI00001", 8);
    fillPos3D(rsi->refPos, 313000000, 1213000000, true);

    /* ---- rtes：2 条，一条富 OPTIONAL，一条极简 ---- */
    fragment(rsi->rtes);
    Asn1List<RTEList_t> rtes(*rsi->rtes);
    {
        RTEData_t* rte = 0;
        rtes.push(rte);
        rte = rtes.back();
        rte->rteId = 1;
        rte->eventType = 37; /* Danger */
        rte->eventSource = EventSource_detection;
        fragment(rte->eventPos);
        fillOffsetLL24(*rte->eventPos, 30, -40, false);
        fragment(rte->eventRadius);
        *rte->eventRadius = 100; /* 0.1m 单位 → 10m */
        fragment(rte->description);
        rte->description->present = Description_PR_textString;
        setIa5(rte->description->textString, "demo-rte-accident");
        fragment(rte->timeDetails);
        fragment(rte->timeDetails->startTime);
        *rte->timeDetails->startTime = 300001;
        fragment(rte->timeDetails->endTime);
        *rte->timeDetails->endTime = 300100;
        fragment(rte->priority);
        setOctet(*rte->priority, "\xE0", 1); /* 较高优先级 */
        fragment(rte->referencePaths);
        Asn1List<ReferencePathList_t> paths(*rte->referencePaths);
        ReferencePath_t* path = 0;
        paths.push(path);
        path = paths.back();
        path->pathRadius = 50;
        Asn1List<PathPointList_t> pts(path->activePath);
        PositionOffsetLLV_t* p0 = 0;
        pts.push(p0);
        p0 = pts.back();
        fillOffsetLL24(*p0, 0, 0, false);
        PositionOffsetLLV_t* p1 = 0;
        pts.push(p1);
        p1 = pts.back();
        fillOffsetLL24(*p1, 10, 10, true);
        fragment(rte->eventConfidence);
        *rte->eventConfidence = 80;
    }
    {
        RTEData_t* rte = 0;
        rtes.push(rte);
        rte = rtes.back();
        rte->rteId = 2;
        rte->eventType = 38; /* UnderConstruction */
        rte->eventSource = EventSource_government;
        /* 其余 OPTIONAL 不填 */
    }

    /* ---- rtss：1 条，带 signPos、GB2312 描述、referenceLinks+lane BIT STRING ---- */
    fragment(rsi->rtss);
    Asn1List<RTSList_t> rtss(*rsi->rtss);
    {
        RTSData_t* rts = 0;
        rtss.push(rts);
        rts = rtss.back();
        rts->rtsId = 11;
        rts->signType = 15; /* Rockfall */
        fragment(rts->signPos);
        fillOffsetLL24(*rts->signPos, 5, 5, false);
        fragment(rts->description);
        rts->description->present = Description_PR_textGB2312;
        /* 两个字节的示意 GB2312 载荷（非真实汉字，仅测 CHOICE 分支） */
        setOctet(rts->description->textGB2312, "\xB9\xFA", 2);
        fragment(rts->referenceLinks);
        Asn1List<ReferenceLinkList_t> links(*rts->referenceLinks);
        ReferenceLink_t* link = 0;
        links.push(link);
        link = links.back();
        link->upstreamNodeId.id = 1;
        link->downstreamNodeId.id = 2;
        fragment(link->upstreamNodeId.region);
        *link->upstreamNodeId.region = 7;
        fragment(link->referenceLanes);
        BitString lanes(*link->referenceLanes, 16);
        lanes.set(ReferenceLanes_lane1);
        lanes.set(ReferenceLanes_lane2);
    }
}

const char* presentName(MessageFrame_PR pr)
{
    switch (pr) {
    case MessageFrame_PR_bsmFrame:
        return "BSM";
    case MessageFrame_PR_mapFrame:
        return "MAP";
    case MessageFrame_PR_rsmFrame:
        return "RSM";
    case MessageFrame_PR_spatFrame:
        return "SPAT";
    case MessageFrame_PR_rsiFrame:
        return "RSI";
    default:
        return "OTHER";
    }
}

void printSummary(const MessageFramePtr& mf, const char* tag)
{
    std::cout << "== " << tag << " (" << presentName(mf->present) << ") ==\n";
    if (mf->present == MessageFrame_PR_rsmFrame && mf->rsmFrame) {
        std::cout << "  participants=" << mf->rsmFrame->participants.list.count << "\n";
        for (int i = 0; i < mf->rsmFrame->participants.list.count; ++i) {
            const ParticipantData_t* p = mf->rsmFrame->participants.list.array[i];
            std::cout << "    [" << i << "] ptcId=" << p->ptcId
                      << " type=" << p->ptcType
                      << " hasId=" << (p->id ? 1 : 0)
                      << " hasTx=" << (p->transmission ? 1 : 0)
                      << " hasAccel=" << (p->accelSet ? 1 : 0)
                      << " hasClass=" << (p->vehicleClass ? 1 : 0) << "\n";
        }
    } else if (mf->present == MessageFrame_PR_spatFrame && mf->spatFrame) {
        std::cout << "  intersections=" << mf->spatFrame->intersections.list.count
                  << " hasName=" << (mf->spatFrame->name ? 1 : 0) << "\n";
        if (mf->spatFrame->intersections.list.count > 0) {
            const IntersectionState_t* is = mf->spatFrame->intersections.list.array[0];
            std::cout << "    phases=" << is->phases.list.count
                      << " statusBytes=" << is->status.size << "\n";
            if (is->phases.list.count > 0) {
                const Phase_t* ph = is->phases.list.array[0];
                std::cout << "    phaseStates=" << ph->phaseStates.list.count << "\n";
                for (int i = 0; i < ph->phaseStates.list.count; ++i) {
                    const PhaseState_t* st = ph->phaseStates.list.array[i];
                    std::cout << "      state[" << i << "] light=" << st->light
                              << " hasTiming=" << (st->timing ? 1 : 0) << "\n";
                }
            }
        }
    } else if (mf->present == MessageFrame_PR_mapFrame && mf->mapFrame) {
        std::cout << "  nodes=" << mf->mapFrame->nodes.list.count
                  << " hasTimeStamp=" << (mf->mapFrame->timeStamp ? 1 : 0) << "\n";
        for (int i = 0; i < mf->mapFrame->nodes.list.count; ++i) {
            const Node_t* n = mf->mapFrame->nodes.list.array[i];
            std::cout << "    [" << i << "] id=" << n->id.id
                      << " hasName=" << (n->name ? 1 : 0)
                      << " hasRegion=" << (n->id.region ? 1 : 0)
                      << " hasElev=" << (n->refPos.elevation ? 1 : 0) << "\n";
        }
    } else if (mf->present == MessageFrame_PR_bsmFrame && mf->bsmFrame) {
        const BasicSafetyMessage_t* bsm = mf->bsmFrame;
        std::cout << "  msgCnt=" << bsm->msgCnt
                  << " hasElev=" << (bsm->pos.elevation ? 1 : 0)
                  << " hasAngle=" << (bsm->angle ? 1 : 0) << "\n";
    } else if (mf->present == MessageFrame_PR_rsiFrame && mf->rsiFrame) {
        const RoadSideInformation_t* rsi = mf->rsiFrame;
        std::cout << "  hasMoy=" << (rsi->moy ? 1 : 0)
                  << " hasRtes=" << (rsi->rtes ? 1 : 0)
                  << " hasRtss=" << (rsi->rtss ? 1 : 0) << "\n";
        if (rsi->rtes) {
            std::cout << "  rtes=" << rsi->rtes->list.count << "\n";
            for (int i = 0; i < rsi->rtes->list.count; ++i) {
                const RTEData_t* r = rsi->rtes->list.array[i];
                std::cout << "    rte[" << i << "] id=" << r->rteId
                          << " hasPos=" << (r->eventPos ? 1 : 0)
                          << " hasDesc=" << (r->description ? 1 : 0)
                          << " hasPaths=" << (r->referencePaths ? 1 : 0)
                          << " descPR=" << (r->description ? r->description->present : 0)
                          << "\n";
            }
        }
        if (rsi->rtss) {
            std::cout << "  rtss=" << rsi->rtss->list.count << "\n";
            for (int i = 0; i < rsi->rtss->list.count; ++i) {
                const RTSData_t* r = rsi->rtss->list.array[i];
                std::cout << "    rts[" << i << "] id=" << r->rtsId
                          << " hasPos=" << (r->signPos ? 1 : 0)
                          << " hasLinks=" << (r->referenceLinks ? 1 : 0)
                          << " descPR=" << (r->description ? r->description->present : 0)
                          << "\n";
                if (r->referenceLinks && r->referenceLinks->list.count > 0) {
                    const ReferenceLink_t* lk = r->referenceLinks->list.array[0];
                    std::cout << "      link lanesBytes="
                              << (lk->referenceLanes ? lk->referenceLanes->size : 0) << "\n";
                }
            }
        }
    }
}

bool checkConstraints(const MessageFramePtr& mf, const char* tag)
{
    std::string err;
    if (!mf.check(err)) {
        std::cerr << tag << " constraint fail: " << err << "\n";
        return false;
    }
    return true;
}

bool verifyRsm(const MessageFramePtr& mf)
{
    if (mf->present != MessageFrame_PR_rsmFrame || !mf->rsmFrame) {
        return false;
    }
    const RoadsideSafetyMessage_t* rsm = mf->rsmFrame;
    if (rsm->participants.list.count != 2) {
        return false;
    }
    const ParticipantData_t* a = rsm->participants.list.array[0];
    const ParticipantData_t* b = rsm->participants.list.array[1];
    return a->id && a->transmission && a->accelSet && a->vehicleClass
        && !b->id && !b->transmission && !b->accelSet && !b->vehicleClass
        && a->pos.offsetLL.present == PositionOffsetLL_PR_position_LL1;
}

bool verifySpat(const MessageFramePtr& mf)
{
    if (mf->present != MessageFrame_PR_spatFrame || !mf->spatFrame) {
        return false;
    }
    const SPAT_t* spat = mf->spatFrame;
    if (!spat->name || spat->intersections.list.count != 1) {
        return false;
    }
    const IntersectionState_t* is = spat->intersections.list.array[0];
    if (is->status.size != 2 || is->phases.list.count != 1) {
        return false;
    }
    const Phase_t* ph = is->phases.list.array[0];
    if (ph->phaseStates.list.count != 2) {
        return false;
    }
    return ph->phaseStates.list.array[0]->timing
        && !ph->phaseStates.list.array[1]->timing
        && ph->phaseStates.list.array[0]->timing->present == TimeChangeDetails_PR_counting;
}

bool verifyMap(const MessageFramePtr& mf)
{
    if (mf->present != MessageFrame_PR_mapFrame || !mf->mapFrame) {
        return false;
    }
    const MapData_t* map = mf->mapFrame;
    if (!map->timeStamp || map->nodes.list.count != 2) {
        return false;
    }
    const Node_t* a = map->nodes.list.array[0];
    const Node_t* b = map->nodes.list.array[1];
    return a->name && a->id.region && !a->refPos.elevation
        && !b->name && !b->id.region && b->refPos.elevation;
}

bool verifyBsm(const MessageFramePtr& mf)
{
    return mf->present == MessageFrame_PR_bsmFrame && mf->bsmFrame
        && mf->bsmFrame->pos.elevation && !mf->bsmFrame->angle
        && mf->bsmFrame->msgCnt == 7;
}

bool verifyRsi(const MessageFramePtr& mf)
{
    if (mf->present != MessageFrame_PR_rsiFrame || !mf->rsiFrame) {
        return false;
    }
    const RoadSideInformation_t* rsi = mf->rsiFrame;
    if (!rsi->moy || !rsi->rtes || !rsi->rtss) {
        return false;
    }
    if (rsi->rtes->list.count != 2 || rsi->rtss->list.count != 1) {
        return false;
    }
    const RTEData_t* a = rsi->rtes->list.array[0];
    const RTEData_t* b = rsi->rtes->list.array[1];
    if (!a->eventPos || !a->description || !a->referencePaths || !a->priority
        || a->description->present != Description_PR_textString) {
        return false;
    }
    if (a->referencePaths->list.count < 1
        || a->referencePaths->list.array[0]->activePath.list.count < 2) {
        return false;
    }
    if (b->eventPos || b->description || b->referencePaths) {
        return false;
    }
    const RTSData_t* rts = rsi->rtss->list.array[0];
    if (!rts->signPos || !rts->description || !rts->referenceLinks
        || rts->description->present != Description_PR_textGB2312) {
        return false;
    }
    if (rts->referenceLinks->list.count < 1
        || !rts->referenceLinks->list.array[0]->referenceLanes
        || rts->referenceLinks->list.array[0]->referenceLanes->size != 2) {
        return false;
    }
    return true;
}

typedef bool (*VerifyFn)(const MessageFramePtr&);

int roundtripOne(const char* name,
                 void (*filler)(MessageFramePtr&),
                 VerifyFn verify,
                 const std::string& outDir)
{
    std::cout << "\n######## " << name << " ########\n";
    MessageFramePtr src;
    filler(src);
    if (!checkConstraints(src, name)) {
        return 1;
    }
    printSummary(src, "source");

    const std::string uperPath = outDir + "/" + name + ".uper";
    const std::string xerPath = outDir + "/" + name + ".xml";
    if (!src.encodeToFile<B_UPER>(uperPath) || !src.encodeToFile<B_XER>(xerPath)) {
        std::cerr << name << ": encodeToFile failed\n";
        return 1;
    }
    std::cout << "wrote " << uperPath << " (" << src.encode<B_UPER>().size() << " B)  "
              << xerPath << " (" << src.encode<B_XER>().size() << " B)\n";

    MessageFramePtr fromUper;
    if (!fromUper.decodeFromFile<B_UPER>(uperPath)) {
        std::cerr << name << ": decode UPER failed\n";
        return 1;
    }
    printSummary(fromUper, "from UPER file");
    if (!verify(fromUper)) {
        std::cerr << name << ": verify after UPER failed\n";
        return 1;
    }

    MessageFramePtr fromXer;
    if (!fromXer.decodeFromFile<B_XER>(xerPath)) {
        std::cerr << name << ": decode XER failed\n";
        return 1;
    }
    printSummary(fromXer, "from XER file");
    if (!verify(fromXer)) {
        std::cerr << name << ": verify after XER failed\n";
        return 1;
    }

    std::cout << "PASS: " << name << " UPER/XER roundtrip\n";
    return 0;
}

/* ---------- 列表所有权：clear / remove / pick / resetField / freeField（配合 valgrind 验证 0 泄漏） ---------- */
RTSData_t* addRts(Asn1List<RTSList_t>& rtss, long id)
{
    RTSData_t* rts = rtss.push();
    rts->rtsId = id;
    rts->signType = 15;
    fragment(rts->signPos);
    fillOffsetLL24(*rts->signPos, 5, 5, true);
    fragment(rts->description);
    rts->description->present = Description_PR_textString;
    setIa5(rts->description->textString, "rts");
    return rts;
}

int listFail(const char* what)
{
    std::cerr << "list ownership: " << what << "\n";
    return 1;
}

int testListOwnership()
{
    std::cout << "\n######## list ownership ########\n";
    MessageFramePtr mf;
    fillRsi(mf);
    RoadSideInformation_t* rsi = mf->rsiFrame;
    Asn1List<RTSList_t> rtss(*rsi->rtss);

    /* RSU transmitRTS 分批场景：装满 16 条 → 编码发送 → 整表清空 → 装下一批，头部不重填 */
    for (long id = 100; rtss.size() < 16; ++id) {
        addRts(rtss, id);
    }
    if (mf.encode<B_UPER>().empty()) {
        return listFail("encode full batch failed");
    }
    rtss.clear();
    if (rtss.size() != 0 || rsi->id.buf == 0) {
        return listFail("clear() should empty list and keep header");
    }
    addRts(rtss, 200);
    addRts(rtss, 201);
    addRts(rtss, 202); /* rtsId INTEGER (0..255) */

    rtss.remove(0);
    rtss.freeElement(rtss.pick(0));
    if (rtss.size() != 1) {
        return listFail("remove/pick count mismatch");
    }

    afl::asn1::resetField(*rsi->rtss);
    afl::asn1::freeField(rsi->rtes);
    if (rsi->rtes != 0 || rtss.size() != 0) {
        return listFail("resetField/freeField state mismatch");
    }

    addRts(rtss, 250);
    if (!checkConstraints(mf, "list-ownership")) {
        return 1;
    }
    MessageFramePtr back;
    if (!back.decode<B_UPER>(mf.encode<B_UPER>())) {
        return listFail("encode/decode after reuse failed");
    }
    const RoadSideInformation_t* r = back->rsiFrame;
    if (!r || r->rtes || !r->rtss || r->rtss->list.count != 1
        || r->rtss->list.array[0]->rtsId != 250) {
        return listFail("content after reuse mismatch");
    }
    std::cout << "PASS: list clear/remove/pick/resetField/freeField\n";
    return 0;
}

int cmdRoundtrip(const std::string& outDir)
{
    int rc = 0;
    rc |= roundtripOne("bsm", fillBsm, verifyBsm, outDir);
    rc |= roundtripOne("rsm", fillRsm, verifyRsm, outDir);
    rc |= roundtripOne("spat", fillSpat, verifySpat, outDir);
    rc |= roundtripOne("map", fillMap, verifyMap, outDir);
    rc |= roundtripOne("rsi", fillRsi, verifyRsi, outDir);
    rc |= testListOwnership();
    if (rc == 0) {
        std::cout << "\nALL PASS: BSM + RSM + SPAT + MAP + RSI + list ownership\n";
    }
    return rc;
}

int cmdEncodeSample(const std::string& outDir)
{
    struct Item {
        const char* name;
        void (*fill)(MessageFramePtr&);
    } items[] = {
        {"bsm", fillBsm},
        {"rsm", fillRsm},
        {"spat", fillSpat},
        {"map", fillMap},
        {"rsi", fillRsi},
    };
    for (size_t i = 0; i < sizeof(items) / sizeof(items[0]); ++i) {
        MessageFramePtr mf;
        items[i].fill(mf);
        const std::string uper = outDir + "/" + items[i].name + ".uper";
        const std::string xer = outDir + "/" + items[i].name + ".xml";
        if (!mf.encodeToFile<B_UPER>(uper) || !mf.encodeToFile<B_XER>(xer)) {
            std::cerr << "encode-sample failed: " << items[i].name << "\n";
            return 1;
        }
        std::cout << "wrote " << uper << " and " << xer << "\n";
    }
    return 0;
}

int cmdDecode(const std::string& codec, const std::string& path)
{
    MessageFramePtr mf;
    bool ok = false;
    if (codec == "uper") {
        ok = mf.decodeFromFile<B_UPER>(path);
    } else if (codec == "xer" || codec == "xml") {
        ok = mf.decodeFromFile<B_XER>(path);
    } else {
        std::cerr << "unknown codec: " << codec << "\n";
        return 1;
    }
    if (!ok) {
        std::cerr << "decode failed: " << path << "\n";
        return 1;
    }
    printSummary(mf, path.c_str());
    const std::string dump = mf.print<16384>();
    if (!dump.empty()) {
        std::cout << dump << "\n";
    }
    return 0;
}

void usage(const char* argv0)
{
    std::cerr
        << "Usage:\n"
        << "  " << argv0 << " [roundtrip] [-o DIR]\n"
        << "  " << argv0 << " encode-sample [-o DIR]\n"
        << "  " << argv0 << " decode --uper FILE\n"
        << "  " << argv0 << " decode --xer  FILE\n";
}

} // namespace

int main(int argc, char** argv)
{
    std::string cmd = "roundtrip";
    std::string outDir = "samples";
    std::string codec;
    std::string path;

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "-h" || a == "--help") {
            usage(argv[0]);
            return 0;
        }
        if (a == "-o" && i + 1 < argc) {
            outDir = argv[++i];
            continue;
        }
        if (a == "--uper" && i + 1 < argc) {
            codec = "uper";
            path = argv[++i];
            continue;
        }
        if (a == "--xer" && i + 1 < argc) {
            codec = "xer";
            path = argv[++i];
            continue;
        }
        if (a == "roundtrip" || a == "encode-sample" || a == "decode") {
            cmd = a;
            continue;
        }
        std::cerr << "unknown arg: " << a << "\n";
        usage(argv[0]);
        return 1;
    }

    if (cmd == "decode") {
        if (codec.empty() || path.empty()) {
            usage(argv[0]);
            return 1;
        }
        return cmdDecode(codec, path);
    }

    if (std::system(("mkdir -p \"" + outDir + "\"").c_str()) != 0) {
        std::cerr << "mkdir failed\n";
        return 1;
    }

    if (cmd == "encode-sample") {
        return cmdEncodeSample(outDir);
    }
    return cmdRoundtrip(outDir);
}
