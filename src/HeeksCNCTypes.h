
// HeeksCNCTypes.h

#pragma once

// NOTE: If adding to this enumeration, please also update the HeeksCNCType() routine.

enum{
	ProgramType = 10001,
	NCCodeBlockType,
	NCCodeType,
	OperationsType,
	ProfileType,
	PocketType,
	ZigZagType,
	DrillingType,
	ToolType,
	ToolsType,
	CounterBoreType,
	TurnRoughType,
	FixtureType,
	FixturesType,
	SpeedReferenceType,
	SpeedReferencesType,
	CuttingRateType,
	PositioningType,
	BOMType,
	TrsfNCCodeType,
	ProbeCentreType,
	ProbeEdgeType,
	ContourType,
	ChamferType,
	InlayType,
	ProbeGridType,
	TagsType,
	TagType,
	ScriptOpType,
	AttachOpType,
	UnattachOpType,
	WaterlineType,
	TappingType,
	HeeksCNCMaximumType
};

#ifndef PI
#define PI 3.14159265358979323846
#endif
