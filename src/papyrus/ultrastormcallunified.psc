ScriptName ultrastormcallunified Extends ActiveMagicEffect
{Unified Storm Call target script. Runs the original tracker and active-searcher modes from one script class.}

GlobalVariable Property MAGProjectileStormVar Auto
GlobalVariable Property SCSOStormMagnitudeVar Auto
GlobalVariable Property SCSOStormActiveCountVar Auto
ImageSpaceModifier Property MagShockCastImod Auto
Activator Property PlacedActivator Auto
Activator Property PlacedTargetActivator Auto
Hazard Property SkyArtSpell Auto
Spell Property SpellRef1 Auto
Spell Property SpellRef2 Auto
Spell Property VisualSpellRef Auto
Sound Property VOCShoutImpactStormCallNear Auto
Sound Property VOCShoutImpactStormCallFar Auto

Float Property fHeight = 3500.0 Auto
Float Property fImodFadeDistance = 3072.0 Auto
Float Property fMaxDelay = 3.00 Auto
Float Property fMinDelay = 1.50 Auto
Float Property fRadiusSmall = 100.0 Auto
Float Property fRadiusWide = 900.0 Auto
{Visual source and miss spread. Overridden per Storm Call word in VMAD: 100 / 300 / 600 world units.}
Float Property fDynamicSearchRadius = 12800.0 Auto
{Dynamic target sphere radius in world units. 12800 world units equals 600 magic-effect feet.}

; Default false preserves the original mod's no-bounty/no-retaliation design.
Bool Property bBlameCaster = False Auto
Bool Property bReverseVisualCast = False Auto
Bool Property bDownwardVisualCast = False Auto
Bool Property bUseReturnStrokeVisual = False Auto
Float Property fReturnStrokeSourceHeight = 64.0 Auto
Float Property fVisualHeight = 2400.0 Auto
Float Property fVisualTargetHeight = 96.0 Auto
Bool Property bUseMagnitudeDamage = True Auto
Float Property fMagnitudeDamageMultiplier = 1.0 Auto
Float Property fMinimumMagnitudeDamage = 0.0 Auto
Int Property iTargetsPerUpdate = 0 Auto
{Optional per-pass target cap. Zero or less means no gameplay cap; the pass stops when target acquisition is exhausted.}
Bool Property bTrackInitialTarget = False Auto
{If true, prefer the magic effect's initial target before dynamic fallback searches.}
Bool Property bPauseInInteriors = True Auto
{If true, keep the storm duration running but suspend all target searches and strikes while the shouter is in an interior cell.}
Int Property iActiveSearchPasses = 1 Auto
{How many active-search passes this instance performs each update. Used to replace old B/C duplicate loops.}
Float Property fActivePassDelayMin = 0.12 Auto
Float Property fActivePassDelayMax = 0.35 Auto
{Random delay inserted between active-search passes to mimic the old B/C independent timing.}
Int Property iRandomSearchAttempts = 10 Auto
{Random searches inside the unified dynamic target sphere after checking the combat target.}

ObjectReference ActivatorRef
ObjectReference ActivatorTargetRef
ObjectReference VisualSourceRef
ObjectReference VisualTargetRef
Actor Shouter
Actor Victim
Float fStormMagnitudeDamage = 0.0
Bool bCasterIsPlayer = False
Bool bHasImod = False
Bool bHasSound = False
Bool bInvalidTarget = False
Bool bKeepUpdating = False
Bool bSharedStormRegistered = False
Spell SpellRef

Event OnInit()
	bKeepUpdating = False
	bInvalidTarget = False
	bSharedStormRegistered = False
	bHasSound = VOCShoutImpactStormCallNear != None || VOCShoutImpactStormCallFar != None
	bHasImod = MagShockCastImod != None
EndEvent

Event OnEffectStart(Actor Target, Actor Caster)
	If PlacedActivator == None
		Debug.Trace("Storm Call Unified: casting activator is missing; storm aborted.", 1)
		Return
	EndIf

	If SpellRef1 == None || SpellRef2 == None || MAGProjectileStormVar == None
		Debug.Trace("Storm Call Unified: a storm spell/global is missing; storm aborted.", 1)
		Return
	EndIf

	Victim = Target
	Shouter = Caster
	bCasterIsPlayer = Caster == Game.GetPlayer()

	ActivatorRef = Caster.PlaceAtMe(PlacedActivator as Form, 1, True, False)
	If bDownwardVisualCast || (bUseReturnStrokeVisual && VisualSpellRef != None)
		VisualSourceRef = Caster.PlaceAtMe(PlacedActivator as Form, 1, True, False)
	EndIf
	If bDownwardVisualCast
		VisualTargetRef = Caster.PlaceAtMe(PlacedActivator as Form, 1, True, False)
	EndIf
	If ActivatorRef == None
		Debug.Trace("Storm Call Unified: failed to create casting activator.", 1)
		Return
	EndIf

	If !bTrackInitialTarget && !bCasterIsPlayer && PlacedTargetActivator != None
		ActivatorTargetRef = Caster.PlaceAtMe(PlacedTargetActivator as Form, 1, True, False)
	EndIf

	BeginSharedStormMagnitude()
	fStormMagnitudeDamage = ResolveMagnitudeDamage()
	SeedSharedMagnitudeDamage(fStormMagnitudeDamage)
	MAGProjectileStormVar.SetValue(1.0)
	bKeepUpdating = True
	RegisterForSingleUpdate(Utility.RandomFloat(fMinDelay, fMaxDelay))
EndEvent

Event OnUpdate()
	If bKeepUpdating && MAGProjectileStormVar != None && MAGProjectileStormVar.GetValue() == 1.0 && ActivatorRef != None && Shouter != None
		Bool pausedForInterior = ShouldPauseForInterior()
		If pausedForInterior
			bInvalidTarget = False
		ElseIf bTrackInitialTarget
			RunTrackerUpdate()
		Else
			RunActiveSearchUpdate()
		EndIf

		If bKeepUpdating
			If pausedForInterior
				RegisterForSingleUpdate(Utility.RandomFloat(fMinDelay, fMaxDelay))
			ElseIf bInvalidTarget && bCasterIsPlayer && !bTrackInitialTarget
				RegisterForSingleUpdate(0.25)
			Else
				RegisterForSingleUpdate(Utility.RandomFloat(fMinDelay, fMaxDelay))
			EndIf
		EndIf
	Else
		StopStorm()
	EndIf
EndEvent

Bool Function ShouldPauseForInterior()
	If !bPauseInInteriors || Shouter == None
		Return False
	EndIf

	Cell shouterCell = Shouter.GetParentCell()
	If shouterCell == None
		Return True
	EndIf

	Return shouterCell.IsInterior()
EndFunction

Function RunTrackerUpdate()
	Actor targetToStrike = Victim
	If !IsValidNewTarget(targetToStrike, None)
		targetToStrike = FindValidTarget()
		Victim = targetToStrike
	EndIf

	If StrikeAvailableTargets(targetToStrike) > 0
		bInvalidTarget = False
	Else
		bInvalidTarget = True
	EndIf
EndFunction

Function RunActiveSearchUpdate()
	Int passCount = iActiveSearchPasses
	If passCount < 1
		passCount = 1
	ElseIf passCount > 3
		passCount = 3
	EndIf

	bInvalidTarget = True
	Int passIndex = 0
	While passIndex < passCount
		MoveCastingSource()

		If bCasterIsPlayer
			Victim = FindValidTarget()
			If StrikeAvailableTargets(Victim) > 0
				bInvalidTarget = False
			EndIf
		Else
			CastMissForNpc()
			bInvalidTarget = False
		EndIf

		passIndex += 1
		If passIndex < passCount
			WaitBetweenActivePasses()
		EndIf
	EndWhile
EndFunction

Function WaitBetweenActivePasses()
	Float delayMin = fActivePassDelayMin
	Float delayMax = fActivePassDelayMax
	If delayMin < 0.0
		delayMin = 0.0
	EndIf
	If delayMax < delayMin
		delayMax = delayMin
	EndIf

	If delayMax > 0.0
		Utility.Wait(Utility.RandomFloat(delayMin, delayMax))
	EndIf
EndFunction

Event OnEffectFinish(Actor Target, Actor Caster)
	StopStorm()
EndEvent

Function MoveCastingSource()
	If ActivatorRef == None || Shouter == None
		Return
	EndIf

	Float xOffset = Utility.RandomFloat(-fRadiusWide, fRadiusWide)
	Float yOffset = Utility.RandomFloat(-fRadiusWide, fRadiusWide)
	ActivatorRef.MoveTo(Shouter as ObjectReference, xOffset, yOffset, fHeight, False)

	If SkyArtSpell != None && ActivatorRef.GetParentCell() != None
		ActivatorRef.PlaceAtMe(SkyArtSpell as Form, 1, False, False)
	EndIf
EndFunction

Actor Function FindValidTarget()
	Return FindValidTargetExcept(None)
EndFunction

Actor Function FindValidTargetExcept(Form[] excludedTargets)
	If Shouter == None
		Return None
	EndIf

	Actor candidate = Shouter.GetCombatTarget()
	If IsValidNewTarget(candidate, excludedTargets)
		Return candidate
	EndIf

	Int tries = 0
	While tries < iRandomSearchAttempts
		candidate = Game.FindRandomActorFromRef(Shouter as ObjectReference, fDynamicSearchRadius)
		If IsValidNewTarget(candidate, excludedTargets)
			Return candidate
		EndIf
		tries += 1
	EndWhile

	Return None
EndFunction

Bool Function IsValidTarget(Actor candidate)
	If candidate == None || Shouter == None
		Return False
	EndIf

	If candidate == Shouter || candidate.IsDead() || candidate.GetParentCell() == None
		Return False
	EndIf

	Return candidate.IsHostileToActor(Shouter)
EndFunction

Bool Function IsWithinDynamicSearchSphere(Actor candidate)
	If candidate == None || Shouter == None
		Return False
	EndIf

	Return Shouter.GetDistance(candidate as ObjectReference) <= fDynamicSearchRadius
EndFunction

Bool Function IsExcludedTarget(Actor candidate, Form[] excludedTargets)
	If candidate == None
		Return True
	EndIf

	If excludedTargets == None
		Return False
	EndIf

	Return excludedTargets.Find(candidate as Form) >= 0
EndFunction

Bool Function IsValidNewTarget(Actor candidate, Form[] excludedTargets)
	If IsExcludedTarget(candidate, excludedTargets)
		Return False
	EndIf

	Return IsValidTarget(candidate) && IsWithinDynamicSearchSphere(candidate)
EndFunction

Int Function StrikeAvailableTargets(Actor preferredTarget)
	Int targetLimit = iTargetsPerUpdate
	Int arrayChunkSize = 16
	Form[] struckTargets = Utility.CreateFormArray(arrayChunkSize)
	If struckTargets == None
		Return 0
	EndIf

	Actor targetToStrike = preferredTarget
	Int struckCount = 0

	While targetLimit <= 0 || struckCount < targetLimit
		If !IsValidNewTarget(targetToStrike, struckTargets)
			targetToStrike = FindValidTargetExcept(struckTargets)
		EndIf

		If !IsValidNewTarget(targetToStrike, struckTargets)
			Return struckCount
		EndIf

		If struckCount >= struckTargets.Length
			Form[] expandedTargets = Utility.ResizeFormArray(struckTargets, struckTargets.Length + arrayChunkSize)
			If expandedTargets == None || expandedTargets.Length <= struckTargets.Length
				Return struckCount
			EndIf
			struckTargets = expandedTargets
		EndIf

		StrikeTarget(targetToStrike, 1)
		struckTargets[struckCount] = targetToStrike as Form
		struckCount += 1
		targetToStrike = None
	EndWhile

	Return struckCount
EndFunction

Function StrikeTarget(Actor targetToStrike, Int lowSpellChance)
	If !IsValidTarget(targetToStrike) || !IsWithinDynamicSearchSphere(targetToStrike) || ActivatorRef == None
		Return
	EndIf

	Float xOffset = Utility.RandomFloat(-fRadiusSmall, fRadiusSmall)
	Float yOffset = Utility.RandomFloat(-fRadiusSmall, fRadiusSmall)
	ActivatorRef.MoveTo(targetToStrike as ObjectReference, xOffset, yOffset, fHeight, False)
	Utility.Wait(0.05)

	If ActivatorRef.GetParentCell() == None
		Return
	EndIf

	SpellRef = SpellRef2
	If Utility.RandomInt(0, 9) < lowSpellChance
		SpellRef = SpellRef1
	EndIf

	Spell visualSpell = GetVisualSpell(SpellRef)
	visualSpell.RemoteCast(ActivatorRef, GetBlameActor(targetToStrike), targetToStrike as ObjectReference)
	ApplyMagnitudeDamage(targetToStrike)
	CastReturnStrokeVisual(targetToStrike)
	CastDownwardVisual(visualSpell, targetToStrike)
	CastReverseVisual(visualSpell, targetToStrike)
	PlayFeedback(targetToStrike as ObjectReference)
EndFunction

Float Function ResolveMagnitudeDamage()
	Float damage = GetMagnitude() * fMagnitudeDamageMultiplier
	If damage < fMinimumMagnitudeDamage
		damage = fMinimumMagnitudeDamage
	EndIf

	Return damage
EndFunction

Function BeginSharedStormMagnitude()
	If bSharedStormRegistered || SCSOStormActiveCountVar == None
		Return
	EndIf

	Float activeCount = SCSOStormActiveCountVar.GetValue()
	If activeCount <= 0.0 && SCSOStormMagnitudeVar != None
		SCSOStormMagnitudeVar.SetValue(0.0)
	EndIf

	SCSOStormActiveCountVar.SetValue(activeCount + 1.0)
	bSharedStormRegistered = True
EndFunction

Function SeedSharedMagnitudeDamage(Float damage)
	If SCSOStormMagnitudeVar == None || damage <= 0.0
		Return
	EndIf

	Float sharedDamage = SCSOStormMagnitudeVar.GetValue()
	If damage > sharedDamage
		SCSOStormMagnitudeVar.SetValue(damage)
	EndIf
EndFunction

Float Function ResolveSharedMagnitudeDamage()
	Float damage = fStormMagnitudeDamage
	If SCSOStormMagnitudeVar != None
		Float sharedDamage = SCSOStormMagnitudeVar.GetValue()
		If sharedDamage > damage
			damage = sharedDamage
		EndIf
	EndIf

	Return damage
EndFunction

Function ApplyMagnitudeDamage(Actor targetToStrike)
	If !bUseMagnitudeDamage || targetToStrike == None
		Return
	EndIf

	Float damage = ResolveSharedMagnitudeDamage()
	If damage <= 0.0
		Return
	EndIf

	targetToStrike.DamageActorValue("Health", damage)
EndFunction

Spell Function GetVisualSpell(Spell fallbackSpell)
	If VisualSpellRef != None
		Return VisualSpellRef
	EndIf

	Return fallbackSpell
EndFunction

Function CastDownwardVisual(Spell spellToCast, Actor targetToStrike)
	If !bDownwardVisualCast || spellToCast == None || targetToStrike == None
		Return
	EndIf

	If VisualSourceRef == None || VisualTargetRef == None
		Return
	EndIf

	Float xOffset = Utility.RandomFloat(-fRadiusSmall, fRadiusSmall)
	Float yOffset = Utility.RandomFloat(-fRadiusSmall, fRadiusSmall)
	VisualSourceRef.MoveTo(targetToStrike as ObjectReference, xOffset, yOffset, fVisualHeight, False)
	VisualTargetRef.MoveTo(targetToStrike as ObjectReference, xOffset, yOffset, fVisualTargetHeight, False)
	Utility.Wait(0.03)

	If VisualSourceRef.GetParentCell() == None || VisualTargetRef.GetParentCell() == None
		Return
	EndIf

	spellToCast.RemoteCast(VisualSourceRef, targetToStrike, VisualTargetRef)
EndFunction

Function CastReverseVisual(Spell spellToCast, Actor targetToStrike)
	If !bReverseVisualCast || spellToCast == None || targetToStrike == None || ActivatorRef == None
		Return
	EndIf

	If targetToStrike.GetParentCell() == None || ActivatorRef.GetParentCell() == None
		Return
	EndIf

	spellToCast.RemoteCast(targetToStrike as ObjectReference, targetToStrike, ActivatorRef)
EndFunction

Function CastReturnStrokeVisual(Actor targetToStrike)
	If !bUseReturnStrokeVisual || VisualSpellRef == None || targetToStrike == None || ActivatorRef == None || VisualSourceRef == None
		Return
	EndIf

	If targetToStrike.GetParentCell() == None || ActivatorRef.GetParentCell() == None
		Return
	EndIf

	VisualSourceRef.MoveTo(targetToStrike as ObjectReference, 0.0, 0.0, fReturnStrokeSourceHeight, False)
	Utility.Wait(0.02)

	If VisualSourceRef.GetParentCell() == None || ActivatorRef.GetParentCell() == None
		Return
	EndIf

	VisualSpellRef.RemoteCast(VisualSourceRef, targetToStrike, ActivatorRef)
EndFunction

Function CastMissForNpc()
	If ActivatorTargetRef == None || ActivatorRef == None || Shouter == None
		Return
	EndIf

	Float xOffset = Utility.RandomFloat(-fRadiusWide, fRadiusWide)
	Float yOffset = Utility.RandomFloat(-fRadiusWide, fRadiusWide)
	ActivatorTargetRef.MoveTo(Shouter as ObjectReference, xOffset, yOffset, 0.0, False)

	If ActivatorTargetRef.GetParentCell() == None || ActivatorRef.GetParentCell() == None
		Return
	EndIf

	SpellRef = SpellRef2
	If Utility.RandomInt(0, 9) < 1
		SpellRef = SpellRef1
	EndIf

	Spell visualSpell = GetVisualSpell(SpellRef)
	visualSpell.RemoteCast(ActivatorRef, Shouter, ActivatorTargetRef)
	PlayFeedback(ActivatorTargetRef)
EndFunction

Actor Function GetBlameActor(Actor targetToStrike)
	If bBlameCaster
		Return Shouter
	EndIf

	Return targetToStrike
EndFunction

Function PlayFeedback(ObjectReference impactRef)
	If impactRef == None || Shouter == None
		Return
	EndIf

	If bHasImod
		Float distanceFactor = fImodFadeDistance - Shouter.GetDistance(impactRef)
		If distanceFactor <= 0.0
			MagShockCastImod.Apply(0.25)
		Else
			distanceFactor /= fImodFadeDistance
			If distanceFactor < 0.25
				distanceFactor = 0.25
			EndIf
			MagShockCastImod.Apply(distanceFactor)
		EndIf
	EndIf

	If bHasSound && impactRef.GetParentCell() != None
		If VOCShoutImpactStormCallNear != None
			VOCShoutImpactStormCallNear.Play(impactRef)
		ElseIf VOCShoutImpactStormCallFar != None
			VOCShoutImpactStormCallFar.Play(impactRef)
		EndIf
	EndIf
EndFunction

Function StopStorm()
	bKeepUpdating = False
	EndSharedStormMagnitude()
	If ActivatorRef != None
		ActivatorRef.Disable(False)
		ActivatorRef.Delete()
		ActivatorRef = None
	EndIf

	If ActivatorTargetRef != None
		ActivatorTargetRef.Disable(False)
		ActivatorTargetRef.Delete()
		ActivatorTargetRef = None
	EndIf

	If VisualSourceRef != None
		VisualSourceRef.Disable(False)
		VisualSourceRef.Delete()
		VisualSourceRef = None
	EndIf

	If VisualTargetRef != None
		VisualTargetRef.Disable(False)
		VisualTargetRef.Delete()
		VisualTargetRef = None
	EndIf
EndFunction

Function EndSharedStormMagnitude()
	If !bSharedStormRegistered
		Return
	EndIf

	bSharedStormRegistered = False
	If SCSOStormActiveCountVar == None
		Return
	EndIf

	Float activeCount = SCSOStormActiveCountVar.GetValue() - 1.0
	If activeCount <= 0.0
		SCSOStormActiveCountVar.SetValue(0.0)
		If SCSOStormMagnitudeVar != None
			SCSOStormMagnitudeVar.SetValue(0.0)
		EndIf
	Else
		SCSOStormActiveCountVar.SetValue(activeCount)
	EndIf
EndFunction
