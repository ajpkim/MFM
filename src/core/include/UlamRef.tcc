/* -*- C++ -*- */
#include "Fail.h"
#include "BitVector.h"
#include "UlamElement.h"
#include "UlamRefMutable.h"

namespace MFM {

  template <class EC>
  UlamRef<EC>::UlamRef(u32 pos, u32 len, BitStorage<EC>& stg,
		       const UlamClass<EC> * effself,
                       const UsageType usage,
		       const UlamContext<EC> & uc)
    : m_uc(uc)
    , m_effSelf(effself)
    , m_stg(stg)
    , m_pos(pos)
    , m_len(len)
    , m_usage(usage)
    , m_posToEff(pos)
  {
    MFM_API_ASSERT_ARG(m_pos + m_len <= m_stg.GetBitSize());
    MFM_API_ASSERT_ARG(m_usage != PRIMITIVE || m_effSelf == 0); // Primitive usage has no effself
    MFM_API_ASSERT_ARG(m_usage != ARRAY || m_effSelf == 0); // Array usage has no effself
    MFM_API_ASSERT_ARG(m_usage != CLASSIC || m_effSelf != 0); // Classic usage has effself

    if(m_usage == ELEMENTAL)
      {
	MFM_API_ASSERT_ARG(pos >= T::ATOM_FIRST_STATE_BIT); //non-negative
	m_posToEff = (u32) (pos - T::ATOM_FIRST_STATE_BIT);
      }
    else if(m_usage == PRIMITIVE)
      {
	m_posToEff = 0u; //no eff self
      }

    if ((m_usage == ATOMIC || m_usage == ELEMENTAL) && !m_effSelf)
      {
	UpdateEffectiveSelf();
      }
  }

  template <class EC>
  UlamRef<EC>::UlamRef(u32 pos, u32 len, u32 postoeff, BitStorage<EC>& stg, const UlamClass<EC> * effself,
            const UsageType usage, const UlamContext<EC> & uc)
    : m_uc(uc)
    , m_effSelf(effself)
    , m_stg(stg)
    , m_pos(pos)
    , m_len(len)
    , m_usage(usage)
    , m_posToEff(postoeff)
  {
    MFM_API_ASSERT_ARG(m_pos + m_len <= m_stg.GetBitSize());
    MFM_API_ASSERT_ARG(m_usage != PRIMITIVE || m_effSelf == 0); // Primitive usage has no effself
    MFM_API_ASSERT_ARG(m_usage != ARRAY || m_effSelf == 0); // Array usage has no effself
    MFM_API_ASSERT_ARG(m_usage != CLASSIC || m_effSelf != 0); // Classic usage has effself

    if ((m_usage == ATOMIC || m_usage == ELEMENTAL) && !m_effSelf)
      {
	UpdateEffectiveSelf();
      }
  }

  template <class EC>
  UlamRef<EC>::UlamRef(const UlamRef<EC> & existing, s32 posincr, u32 len, const UlamClass<EC> * effself, const UsageType usage)
    : m_uc(existing.m_uc)
    , m_effSelf(effself)
    , m_stg(existing.m_stg)
    , m_len(len)
  {
    s32 newpos = posincr + (s32) existing.GetPos(); //e.g. pos -25 to start of atom of element ref
    MFM_API_ASSERT_ARG(newpos >= 0); //non-negative
    m_pos = (u32) newpos; //save as unsigned

    m_usage = usage; //save

    MFM_API_ASSERT_ARG(m_pos + m_len <= m_stg.GetBitSize());
    MFM_API_ASSERT_ARG(existing.m_usage != PRIMITIVE || m_usage == existing.m_usage);  // derived from PRIMITIVE can't change usage type
    MFM_API_ASSERT_ARG(m_usage != ARRAY || m_effSelf == 0); // Array usage has no effself
    MFM_API_ASSERT_ARG(m_usage != CLASSIC || m_effSelf != 0); // Classic usage has effself

    if ((m_usage == ATOMIC || m_usage == ELEMENTAL) && !m_effSelf)
    {
      UpdateEffectiveSelf();
    }

    if((usage == ATOMIC) && (existing.m_usage == ELEMENTAL))
      m_posToEff = 0u; //== pos + t::atom_first_state_bit
    else if((usage == ELEMENTAL) && (existing.m_usage == ATOMIC))
      m_posToEff = 0u; //== pos + t::atom_first_state_bit
    else if((usage == CLASSIC) && (existing.m_usage == ATOMIC))
      m_posToEff = m_pos - T::ATOM_FIRST_STATE_BIT; //== pos - t::atom_first_state_bit (t41360)
    else if(m_effSelf && (m_effSelf != existing.m_effSelf))
      m_posToEff = 0u; //data member, new effSelf
    else if(usage == PRIMITIVE)
      m_posToEff = 0u;
    else //base class, same eff self
      {
	//negative when the new base is a subclass of old base (t41325)
	//MFM_API_ASSERT_ARG(posincr >= 0); //non-negative
	m_posToEff = existing.m_posToEff + posincr; //subtract from newpos for eff self pos
      }
  }

  template <class EC>
  UlamRef<EC>::UlamRef(const UlamRef<EC> & existing, s32 posincr, u32 len)
    : m_uc(existing.m_uc)
    , m_effSelf(existing.m_effSelf)
    , m_stg(existing.m_stg)
    , m_len(len)
    , m_usage(existing.m_usage)
    , m_posToEff(existing.m_posToEff + posincr)
  {
    s32 newpos = posincr + (s32) existing.GetPos(); //e.g. pos -25 to start of atom of element ref
    MFM_API_ASSERT_ARG(newpos >= 0); //non-negative
    m_pos = (u32) newpos; //save as unsigned

    MFM_API_ASSERT_ARG(m_pos + m_len <= m_stg.GetBitSize());
    if ((m_usage == ATOMIC || m_usage == ELEMENTAL) && !m_effSelf)
    {
      UpdateEffectiveSelf();
    }
  }

  template <class EC>
  UlamRef<EC>::UlamRef(const UlamRef<EC> & existing, s32 effselfoffset, u32 len, bool applydelta)
    : m_uc(existing.m_uc)
    , m_effSelf(existing.m_effSelf)
    , m_stg(existing.m_stg)
    , m_len(len)
    , m_usage(existing.m_usage)
  {
    MFM_API_ASSERT_ARG(effselfoffset >= 0); //non-negative
    MFM_API_ASSERT_ARG(applydelta); //always true, de-ambiguity arg

    //virtual func override class ref, from existing calling ref
    ApplyDelta(existing.GetEffectiveSelfPos(), effselfoffset, len);

    if ((m_usage == ATOMIC || m_usage == ELEMENTAL) && !m_effSelf)
    {
      UpdateEffectiveSelf();
    }
  }

  template <class EC>
  UlamRef<EC>::UlamRef(const UlamRef<EC> & existing, u32 vownedfuncidx, const UlamClass<EC> & origclass, VfuncPtr & vfuncref)
    : m_uc(existing.m_uc)
    , m_effSelf(existing.m_effSelf)
    , m_stg(existing.m_stg)
    , m_len(existing.m_len)
    , m_usage(existing.m_usage)
  {
    InitUlamRefForVirtualFuncCall(existing, vownedfuncidx, origclass.GetRegistrationNumber(), vfuncref);

    if ((m_usage == ATOMIC || m_usage == ELEMENTAL) && !m_effSelf)
    {
      UpdateEffectiveSelf();
    }
  }

  template <class EC>
  UlamRef<EC>::UlamRef(const UlamRef<EC> & existing, u32 vownedfuncidx, u32 origclassregnum, bool applydelta, VfuncPtr & vfuncref)
    : m_uc(existing.m_uc)
    , m_effSelf(existing.m_effSelf)
    , m_stg(existing.m_stg)
    , m_len(existing.m_len)
    , m_usage(existing.m_usage)
  {
    //applydelta is de-ambiguity arg
    InitUlamRefForVirtualFuncCall(existing, vownedfuncidx, origclassregnum, vfuncref);
    if ((m_usage == ATOMIC || m_usage == ELEMENTAL) && !m_effSelf)
    {
      UpdateEffectiveSelf();
    }
  }

  template <class EC>
  UlamRef<EC>::UlamRef(const UlamRefMutable<EC> & muter)
    : m_uc(*checknonnulluc(muter.GetContextPtr()))
    , m_effSelf(muter.GetEffectiveSelfPtr())
    , m_stg(*checknonnullstg(muter.GetBitStoragePtr()))
    , m_pos(muter.GetPos())
    , m_len(muter.GetLen())
    , m_usage(muter.GetUsageType())
    , m_posToEff(muter.GetPosToEffectiveSelf())
  { }


  template <class EC>
  void UlamRef<EC>::InitUlamRefForVirtualFuncCall(const UlamRef<EC> & ur, u32 vownedfuncidx, u32 origclassregnum, VfuncPtr & vfuncref)
  {
    const UlamClass<EC> * effSelf = ur.GetEffectiveSelf();
    MFM_API_ASSERT_NONNULL(effSelf);

    const u32 origclassvtstart = effSelf->GetVTStartOffsetForClassByRegNum(origclassregnum);

    vfuncref = effSelf->getVTableEntry(vownedfuncidx + origclassvtstart); //return ref to virtual function ptr

    const UlamClass<EC> * ovclassptr = effSelf->getVTableEntryUlamClassPtr(vownedfuncidx + origclassvtstart);
    MFM_API_ASSERT_NONNULL(ovclassptr);

    const u32 ovclassrelpos = effSelf->internalCMethodImplementingGetRelativePositionOfBaseClass(ovclassptr);
    MFM_API_ASSERT(ovclassrelpos >= 0, PURE_VIRTUAL_CALLED);

    ApplyDelta(ur.GetEffectiveSelfPos(), ovclassrelpos, ovclassptr->GetClassLength());

    m_usage = ovclassptr->AsUlamElement() ? ELEMENTAL : CLASSIC;

  } //InitUlamRefForVirtualFuncCall

  template <class EC>
  void UlamRef<EC>::ApplyDelta(s32 existingeffselfpos, s32 effselfoffset, u32 len)
  {
    MFM_API_ASSERT_ARG(effselfoffset >= 0); //non-negative

    //virtual func override class ref, from existing calling ref
    s32 newpos = effselfoffset + existingeffselfpos;
    MFM_API_ASSERT_ARG(newpos >= 0); //non-negative
    m_pos = (u32) newpos; //save as unsigned
    m_posToEff = (u32) effselfoffset; //subtract from newpos for pos of effself
    m_len = len;
    MFM_API_ASSERT_ARG((m_pos + m_len - m_posToEff) <= m_stg.GetBitSize());
  }

  template <class EC>
  void UlamRef<EC>::UpdateEffectiveSelf()
  {
    m_effSelf = LookupUlamElementTypeFromAtom();
  }

  template <class EC>
  void UlamRef<EC>::CheckEffectiveSelf() const
  {
    if (m_usage == ATOMIC || m_usage == ELEMENTAL)
    {
      const UlamClass<EC> * eltptr = LookupUlamElementTypeFromAtom();
      MFM_API_ASSERT((eltptr->internalCMethodImplementingIs(m_effSelf)), STALE_ATOM_REF);
    }
  }

  template <class EC>
  const UlamClass<EC>* UlamRef<EC>::LookupUlamElementTypeFromAtom() const
  {
    MFM_API_ASSERT_STATE(m_usage == ATOMIC || m_usage == ELEMENTAL);
    T a = ReadAtom();
    MFM_API_ASSERT(a.IsSane(),INCONSISTENT_ATOM);
    u32 etype = a.GetType();
    const UlamClass<EC> * eltptr = m_uc.LookupUlamElementTypeFromContext(etype);
    MFM_API_ASSERT_STATE(eltptr);
    return eltptr;
  }

  template <class EC>
  u32 UlamRef<EC>::GetType() const
  {
    const UlamClass<EC> * effSelf = GetEffectiveSelf();
    MFM_API_ASSERT_ARG(effSelf);
    const UlamElement<EC> * eltptr = effSelf->AsUlamElement();
    if(!eltptr) return T::ATOM_UNDEFINED_TYPE; //quark
    return eltptr->GetType();
  } //GetType

  template <class EC>
  typename EC::ATOM_CONFIG::ATOM_TYPE UlamRef<EC>::CreateAtom() const
  {
    const UlamClass<EC> * effSelf = GetEffectiveSelf();
    MFM_API_ASSERT_ARG(effSelf);
    const UlamElement<EC> * eltptr = effSelf->AsUlamElement();
    if(!eltptr) FAIL(ILLEGAL_ARGUMENT);
    u32 len = eltptr->GetClassLength();
    AtomBitStorage<EC> atmp(eltptr->GetDefaultAtom());
    atmp.WriteBig(0u + T::ATOM_FIRST_STATE_BIT, len, m_stg.ReadBig(GetPos(), len));
    return atmp.ReadAtom();
  }

  template <class EC>
  void UlamRef<EC>::Print(const UlamClassRegistry<EC>&uc, ByteSink& bs, u32 printFlags) const
  {
    if (!m_effSelf)
    {
      bs.Printf("UlamRef[pos=%d,len=%d,NULL]", m_pos, m_len);
      return;
    }

    const UlamElement<EC> * ue = m_effSelf->AsUlamElement();
    if (ue)
    {
      const T atom = this->ReadAtom();
      ue->Print(uc, bs, atom, printFlags, m_pos);
      return;
    }

    // If this isn't an ulam element, MFM doesn't have name info for
    // its type, but we can still print its class members
    m_effSelf->PrintClassMembers(uc, bs, m_stg, printFlags, GetEffectiveSelfPos());
  }

} //MFM
