#ifndef CPUFLAGS_INCLUDE_H
#define CPUFLAGS_INCLUDE_H

enum cpu_flag_t {
	/* (EAX 01h) ECX features*/
	CPUFLAG_SSE3 = 0,               /**< SSE3 */
	CPUFLAG_PCLMULQDQ,              /**< PCLMULQDQ */
	CPUFLAG_DTES64,                 /**< DTES64 */
	CPUFLAG_MONITOR,                /**< MONITOR */
	CPUFLAG_DS_CPL,                 /**< DS_CPL */
	CPUFLAG_VMX,                    /**< VMX */
	CPUFLAG_SMX,                    /**< SMX */
	CPUFLAG_EIST,                   /**< EIST */
	CPUFLAG_TM2,                    /**< TM2 */
	CPUFLAG_SSSE3,                  /**< SSSE3 */
	CPUFLAG_CNXT_ID,                /**< CNXT_ID */
	CPUFLAG_FMA,                    /**< FMA */
	CPUFLAG_CMPXCHG16B,             /**< CMPXCHG16B */
	CPUFLAG_XTPR,                   /**< XTPR */
	CPUFLAG_PDCM,                   /**< PDCM */
	CPUFLAG_PCID,                   /**< PCID */
	CPUFLAG_DCA,                    /**< DCA */
	CPUFLAG_SSE4_1,                 /**< SSE4_1 */
	CPUFLAG_SSE4_2,                 /**< SSE4_2 */
	CPUFLAG_X2APIC,                 /**< X2APIC */
	CPUFLAG_MOVBE,                  /**< MOVBE */
	CPUFLAG_POPCNT,                 /**< POPCNT */
	CPUFLAG_TSC_DEADLINE,           /**< TSC_DEADLINE */
	CPUFLAG_AES,                    /**< AES */
	CPUFLAG_XSAVE,                  /**< XSAVE */
	CPUFLAG_OSXSAVE,                /**< OSXSAVE */
	CPUFLAG_AVX,                    /**< AVX */
	CPUFLAG_F16C,                   /**< F16C */
	CPUFLAG_RDRAND,                 /**< RDRAND */
	CPUFLAG_HYPERVISOR,             /**< Running in a VM */

	/* (EAX 01h) EDX features */
	CPUFLAG_FPU,                    /**< FPU */
	CPUFLAG_VME,                    /**< VME */
	CPUFLAG_DE,                     /**< DE */
	CPUFLAG_PSE,                    /**< PSE */
	CPUFLAG_TSC,                    /**< TSC */
	CPUFLAG_MSR,                    /**< MSR */
	CPUFLAG_PAE,                    /**< PAE */
	CPUFLAG_MCE,                    /**< MCE */
	CPUFLAG_CX8,                    /**< CX8 */
	CPUFLAG_APIC,                   /**< APIC */
	CPUFLAG_SEP,                    /**< SEP */
	CPUFLAG_MTRR,                   /**< MTRR */
	CPUFLAG_PGE,                    /**< PGE */
	CPUFLAG_MCA,                    /**< MCA */
	CPUFLAG_CMOV,                   /**< CMOV */
	CPUFLAG_PAT,                    /**< PAT */
	CPUFLAG_PSE36,                  /**< PSE36 */
	CPUFLAG_PSN,                    /**< PSN */
	CPUFLAG_CLFSH,                  /**< CLFSH */
	CPUFLAG_DS,                     /**< DS */
	CPUFLAG_ACPI,                   /**< ACPI */
	CPUFLAG_MMX,                    /**< MMX */
	CPUFLAG_FXSR,                   /**< FXSR */
	CPUFLAG_SSE,                    /**< SSE */
	CPUFLAG_SSE2,                   /**< SSE2 */
	CPUFLAG_SS,                     /**< SS */
	CPUFLAG_HTT,                    /**< HTT */
	CPUFLAG_TM,                     /**< TM */
	CPUFLAG_PBE,                    /**< PBE */

	/* (EAX 06h) EAX features */
	CPUFLAG_DIGTEMP,                /**< DIGTEMP */
	CPUFLAG_TRBOBST,                /**< TRBOBST */
	CPUFLAG_ARAT,                   /**< ARAT */
	CPUFLAG_PLN,                    /**< PLN */
	CPUFLAG_ECMD,                   /**< ECMD */
	CPUFLAG_PTM,                    /**< PTM */

	/* (EAX 06h) ECX features */
	CPUFLAG_MPERF_APERF_MSR,        /**< MPERF_APERF_MSR */
	CPUFLAG_ACNT2,                  /**< ACNT2 */
	CPUFLAG_ENERGY_EFF,             /**< ENERGY_EFF */

	/* (EAX 07h, ECX 0h) EBX features */
	CPUFLAG_FSGSBASE,               /**< FSGSBASE */
	CPUFLAG_BMI1,                   /**< BMI1 */
	CPUFLAG_HLE,                    /**< Hardware Lock elision */
	CPUFLAG_AVX2,                   /**< AVX2 */
	CPUFLAG_SMEP,                   /**< SMEP */
	CPUFLAG_BMI2,                   /**< BMI2 */
	CPUFLAG_ERMS,                   /**< ERMS */
	CPUFLAG_INVPCID,                /**< INVPCID */
	CPUFLAG_RTM,                    /**< Transactional memory */
	CPUFLAG_AVX512F,                /**< AVX512F */

	/* (EAX 80000001h) ECX features */
	CPUFLAG_LAHF_SAHF,              /**< LAHF_SAHF */
	CPUFLAG_LZCNT,                  /**< LZCNT */

	/* (EAX 80000001h) EDX features */
	CPUFLAG_SYSCALL,                /**< SYSCALL */
	CPUFLAG_XD,                     /**< XD */
	CPUFLAG_1GB_PG,                 /**< 1GB_PG */
	CPUFLAG_RDTSCP,                 /**< RDTSCP */
	CPUFLAG_EM64T,                  /**< EM64T */

	/* (EAX 80000007h) EDX features */
	CPUFLAG_INVTSC,                 /**< INVTSC */

	/* The last item */
	CPUFLAG_NUMFLAGS,               /**< This should always be the last! */
};

/**
 * Function for checking a CPU flag availability
 *
 * @param feature
 *     CPU flag to query CPU for
 * @return
 *     1 if flag is available
 *     0 if flag is not available
 *     -1 if flag is invalid
 */
__extension__
int
cpu_get_flag_enabled(enum cpu_flag_t feature);

#endif // CPUFLAGS_INCLUDE_H