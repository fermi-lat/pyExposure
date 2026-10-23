---
package: pyExposure
summary: The gtexposure tool and the small Exposure library behind it, which add a spectrally weighted EXPOSURE column (cm² s) to a gtbin light curve.
layer: analysis
depends_on: [Likelihood, irfs, dataSubselector, optimizers, astro, tip, st_app, st_stream, st_facilities, xmlBase, CLHEP]
used_by: []
last_reviewed_commit: e14e7d1
---

# pyExposure Architecture

**pyExposure** builds one tool, **`gtexposure`**. It takes a light curve
written by `gtbin` (`algorithm=LC`), computes the LAT exposure toward the
target for each time bin, weights it by an assumed source spectrum, and
writes the result **into the input file** as an `EXPOSURE` column. Dividing
`COUNTS` by `EXPOSURE` then gives a flux in photons cm⁻² s⁻¹ over the
selected energy range.

Despite the name, the package has **no working Python interface**. The SWIG
module (`src/pyExposure.i`) has not been built since 2009 (`doc/release.notes`,
"disable swig build"), CMake does not build it, and the two scripts in
`python/` are neither installed nor runnable (see *Public interfaces*). Treat
pyExposure as a C++ tool package.

## Purpose and scope

**Does:**
- For each time bin of a light curve, integrate effective area × livetime
  over the FT2 intervals inside that bin and inside the light curve's GTI
  (`pyExposure::Exposure`).
- Optionally multiply the effective area by the PSF containment within the
  event-selection cone (the "aperture correction").
- Average the exposure over energy, weighted by a spectral model: either a
  power law with index `specin`, or a named source's `<spectrum>` from a
  `gtlike`-style XML model.
- Read the target position, cone radius, energy range and (for
  `irfs=CALDB`) the IRF name from the light curve's DSS keywords.

**Does not:**
- Bin events or create the light curve. `gtbin` (`evtbin`) does that; the
  light curve must already carry the DSS keywords that `gtselect` wrote.
- Compute exposure maps or livetime cubes. `Likelihood` (`gtexpmap`,
  `gtltcube`, `gtexpcube2`) does that.
- Provide a Python API. See the Python notes below.

## Position in the system

| Direction | Package | Why |
|-----------|---------|-----|
| Depends on | `Likelihood` | `ScData` (FT2 reader), `LikeExposure::acceptInterval` (overlap of an FT2 interval with time cuts and GTIs), and the spectral function classes registered in `gtexposure`'s function factory (`PowerLaw2`, `ExpCutoff`, `LogParabola`, `PLSuperExpCutoff`1–4, `BrokenPowerLaw2`, `BPLExpCutoff`, `FileFunction`, `MapCubeFunction2`). |
| Depends on | `irfs` (`irfLoader`, `irfInterface`) | `irfLoader::Loader::respIds()` maps an IRF name to its event-type components; `IAeff`, `IPsf::angularIntegral` and `IEfficiencyFactor` give the effective area, aperture fraction and livetime efficiency. |
| Depends on | `dataSubselector` | `Cuts` reads the light curve's DSS keywords (`SkyConeCut`, `RangeCut` on `ENERGY`, `CALDB_implied_irfs()`); `GtiCut` reads its GTI. |
| Depends on | `optimizers` | `Function`, `FunctionFactory`, `dArg`. The factory's default constructor already registers `PowerLaw`, `BrokenPowerLaw`, `Gaussian`, `AbsEdge` and `ConstantValue`. |
| Depends on | `xmlBase` | Only the vendored `rapidxml.hpp` header, used by `gtexposure` to parse the source model. No library symbols. |
| Depends on | `astro` | `SkyDir` for the target and spacecraft axes. |
| Depends on | `tip` | Reads `TIME`/`TIMEDEL` from the `RATE` table and appends the `EXPOSURE` column. |
| Depends on | `st_app`, `st_stream` | Tool framework and output. `hoops` comes through `st_app`. |
| Depends on | `st_facilities` | `Util::resolve_fits_files` (expands `@list` FT2 input), `Util::file_ok`, `Util::expectedException`. |
| Depends on | CLHEP | `Hep3Vector` in `Exposure::effArea`. Not in the package's link list; it arrives through `Likelihood`/`astro`. |
| Used by | — | No package includes `pyExposure/Exposure.h` or links the library. `BayesianBlocks/python/create_lc_file.py` **runs the `gtexposure` tool** through `sane`'s `GtApp`. |

**Link list vs. use.** `CMakeLists.txt` links everything `PUBLIC` to the
`pyExposure` library (and lists `dataSubselector` twice). The library source
(`src/Exposure.cxx`) uses only `Likelihood`, `irfs`, `astro`,
`st_facilities` and CLHEP; `dataSubselector`, `optimizers`, `xmlBase`,
`tip`, `st_app` and `st_stream` are needed only by the `gtexposure`
executable. `evtbin` appears only in the unbuilt SWIG file.

**Tier.** The package is listed in the `python` tier, but it builds a C++
library and a `gt` tool, has no Python module, and depends on `Likelihood`.
By what it contains it belongs in `analysis`, which is what this doc uses.

## Key abstractions

| Type | Where | Role |
|------|-------|------|
| `pyExposure::Exposure` | `pyExposure/Exposure.h`, `src/Exposure.cxx` | Given an FT2 file, time-bin boundaries, GTIs, an energy grid, a sky position, a cone radius and an IRF name, computes `values()[timeBin][energy]` in cm² s at construction. `value(t, E)` interpolates in log E. |
| `GtExposure` | `src/gtexposure/gtexposure.cxx` | The `st_app` application. Reads parameters and DSS keywords, builds an `Exposure`, builds the weighting function, writes the column. |

The exposure for time bin *j* and energy *E* is

```
sum over FT2 rows i overlapping bin j:
    ½ [A(tstart_i, E) + A(tstop_i, E)] × LIVETIME_i × fraction_i
```

where `fraction_i` is the part of `[tstart_i, tstop_i)` that lies inside both
the bin and the GTI (`LikeExposure::acceptInterval`), and

```
A(t, E) = Σ over event types k:  Aeff_k(E, θ, φ) × C_k(E, θ, φ, radius) × eff_k(E, livetime fraction)
```

with θ, φ the target's angles in the spacecraft frame at time `t`,
`C_k` the PSF containment within `radius` (1 when `radius ≥ 180`), and
`eff_k` the livetime efficiency factor if the IRF provides one.

## Data flow and control flow

```
GtExposure::run
  promptForParameters   infile, scfile, irfs, srcmdl, then specin (srcmdl=none) or target
  parseDssKeywords      dataSubselector::Cuts(infile, "RATE")
                          SkyCone  → ra, dec (and radius if apcorr=yes)
                          range on ENERGY → emin, emax
                          otherwise the hidden ra/dec/rad/emin/emax parameters are used
  setExposure           enumbins log-spaced energies in [emin, emax]
                        time edges from RATE: TIME − TIMEDEL/2 per row, plus last TIME + TIMEDEL/2
                        irfs=CALDB → Cuts::CALDB_implied_irfs() (needs an EVENT_CLASS DSS cut)
                        GTIs from GtiCut(infile)
                        Exposure(scfile, edges, gtis, energies, ra, dec, radius, irfs)
                          irfLoader::Loader::go(); create one Irfs per event-type component
                          ScData::readData for [tmin, tmax + max(2·last bin width, 60 s)]
                          integrateExposure()
  prepareModel          srcmdl=none → PowerLaw2 with Index = specin
                        else rapidxml parse, find <source name=target>, create <spectrum type=…>, setParams
  performSpectralWeighting
                        EXPOSURE_j = ∫ ε_j(E) F(E) dE / ∫ F(E) dE   (trapezoid on the energy grid)
  writeExposure         editTable(infile, "RATE"); appendField("EXPOSURE","E") unless present;
                        TUNITn = "cm**2 s"; write one value per row
```

## Public interfaces

### Applications

| Tool | Par file | Notes |
|------|----------|-------|
| `gtexposure` | `pfiles/gtexposure.par` | Prompted: `infile`, `scfile`, `irfs` (default `CALDB`), `srcmdl` (default `none`), then `specin` (default −2.1) or `target`. Hidden: `ra`, `dec`, `rad`, `apcorr` (yes), `emin` (30), `emax` (2e5), `enumbins` (30, 2–200), plus the standard `chatter`/`clobber`/`debug`/`gui`/`mode`. |

There is no `outfile`: the tool **modifies `infile` in place**. `clobber` is
not consulted. Standard tool behaviour (exit status, hidden parameters,
`TRAP_FPE`) is described in `src/st_app/doc/architecture/`; par-file lookup
in `src/ape/doc/architecture/parameter-files.md`. User help is in
`fermitools-fhelp` (`gtexposure.txt`).

Accepted `<spectrum type=…>` values in `srcmdl`: `PowerLaw`,
`BrokenPowerLaw`, `Gaussian`, `AbsEdge`, `ConstantValue` (optimizers
defaults), and `PowerLaw2`, `BrokenPowerLaw2`, `BPLExpCutoff`, `ExpCutoff`,
`LogParabola`, `PLSuperExpCutoff`, `PLSuperExpCutoff2`–`4`,
`FileFunction`, `MapCubeFunction` (registered in
`GtExposure::prepareFunctionFactory`). Any other type, including
`DMFitFunction`, `ScaleFactor::…` and `EblAtten::…`, is rejected by the
factory. `FileFunction` does not work; see gotchas.

### C++ library

`pyExposure/Exposure.h` is installed to `include/fermitools/pyExposure`, but
no other package uses it. The constructor does all the work:

```cpp
pyExposure::Exposure exp("ft2.fits",            // or @list
                         tedges,                 // N+1 bin edges (MET)
                         gtis,                   // vector<pair<start,stop>>
                         energies,               // ascending, MeV
                         ra, dec, radius_deg,    // radius ≥ 180 → no aperture correction
                         "P8R3_SOURCE_V3");      // a name known to irfLoader
const auto & ev = exp.values();                  // [time bin][energy], cm² s
```

### Python

None that works:
- `src/pyExposure.i` (and its older SCons-era twin `src/pyExposure.in`)
  would wrap `Exposure`, `evtbin::Gti` and the `dataSubselector` cut
  classes. It is not built by CMake and was switched off in SCons in 2009.
  Its `Cuts.castAsSkyConeCut`/`castAsRangeCut` helpers test the input
  pointer instead of the cast result, so a failed cast would return `None`
  rather than raise.
- `python/flux_lc.py` imports the missing `pyExposure` module, `numarray`,
  `FunctionWrapper` and `hippoplotter`. `python/light_curves.py` uses
  `pyfits` and `FitsNTuple` and defaults to `P6_V1_SOURCE` IRFs. Neither is
  installed by CMake (no `install(DIRECTORY python/ …)`). Both are dead
  code.

### Environment variables

No direct `getenv`. IRF discovery uses `CALDB` (through `irfs`) and par
files use `PFILES`; see "Runtime environment" in ScienceTools'
`docs/architecture/README.md`.

### File formats

- **Inputs:** a `gtbin` light curve with a `RATE` table (`TIME` = bin
  midpoint, `TIMEDEL`), a `GTI` extension and DSS keywords; FT2 (`SC_DATA`,
  `@list` accepted); optionally an XML source model.
- **Output:** the same light curve, with a float `EXPOSURE` column
  (`TUNIT` = `cm**2 s`) added or overwritten in `RATE`.

## Build and test

- **CMake targets:** `pyExposure` (shared library, `src/Exposure.cxx`) and
  `gtexposure`. Both are compiled with `TRAP_FPE` on Linux.
- **Install:** headers to `include/fermitools/pyExposure`, `pfiles/` to
  `syspfiles`, library and executable to `lib`/`bin`. `python/` and the
  SWIG files are not installed. There is no `data/` directory.
- **Tests:** none. The package has no test program, no test data and no
  entry in `recipe/tests/ST-unit-test`. `gtexposure` is not exercised by CI.
  A quick manual check is to bin `src/evtbin/data/ft1tiny.fits` with
  `gtbin algorithm=LC tstart=223850000 tstop=223950000 dtime=20000` and run
  `gtexposure` on the result with `src/evtbin/data/ft2tiny.fits` and an
  explicit `irfs` (the tiny files have no `EVENT_CLASS` DSS cut, so
  `irfs=CALDB` fails). Run it in a scratch directory; the light curve is
  modified in place.
- **Legacy:** `SConscript`, `pyExposureLib.py`, `.cvsignore` and
  `src/pyExposure.in` belong to the old SCons/CMT builds.

## Invariants and gotchas

**Inputs and defaults**
- The tool **overwrites `infile`**. Re-running it replaces the `EXPOSURE`
  column. Keep a copy if the original light curve matters.
- Position, cone radius and energy range come from the DSS keywords when
  present and override the hidden parameters silently. If the light curve
  has **no `SkyCone` DSS cut**, the hidden `ra`/`dec` defaults (0, 0) are
  used without a warning, so the exposure is computed for the wrong sky
  position.
- With `apcorr=yes` (the default) the aperture correction uses the DSS cone
  radius. With no cone in the DSS keywords, `rad` defaults to 180 and no
  correction is applied.
- `irfs=CALDB` needs an `EVENT_CLASS` bitmask DSS cut in the light curve and
  fails otherwise (exit status 1, confirmed at runtime).
- `specin` is the photon index of `dN/dE ∝ E^specin`, so it is negative
  (default −2.1). Only the shape matters; the normalization cancels.

**Crashes instead of errors**
- An IRF name unknown to `irfLoader` is **undefined behaviour**:
  `Exposure`'s constructor dereferences `respIds().find(irfs)` without
  checking for `end()`. `gtexposure irfs=NOT_AN_IRF` segfaults (exit 139)
  (confirmed at runtime with the Fermitools 2.5.1 build).
- `srcmdl` with a `FileFunction` spectrum **segfaults** (exit 139, confirmed
  at runtime with Fermitools 2.5.1). `Function::setParams` reads only the
  `<parameter>` elements; the `file` attribute is never passed to
  `FileFunction::readFunction` (which `Likelihood::SourceFactory` does), so
  the function has no table. HEAD's rapidxml version of `prepareModel` has
  the same logic.
- When `CALDB` and `FERMI_INST_DIR` were not set up by conda activation,
  `gtexposure` aborted (exit 134) before opening the light curve, with
  `File not found: …/caldb/software/tools/alias_config.fits` (observed at
  runtime; the uncaught throw comes from IRF loading, apparently outside
  `st_app`'s `try`, so the message is `terminate called …` rather than
  `Caught … at the top level`).

**Time binning**
- Bin edges are rebuilt from the `RATE` table as `TIME − TIMEDEL/2` for each
  row plus a final `TIME + TIMEDEL/2`. Each bin therefore ends where the
  next one starts. For non-contiguous bins (for example a `tbinalg=FILE`
  definition with gaps), the gap is counted in the preceding bin's exposure
  where it lies inside the GTI (from reading the code; not checked at
  runtime).
- An empty `RATE` table is not checked for; `getLcTimes` dereferences
  `table->begin()` (from reading the code).
- If a bin edge lies outside the FT2 coverage, `ScData::time_index` throws
  and `integrateExposure` falls back to scanning every FT2 row read for that
  bin. Only `[tmin, tmax + padding]` was read, so the result is still
  bounded, but no warning is printed when the FT2 file does not cover the
  light curve.

**Numerics**
- Effective area is evaluated at both ends of each FT2 interval and
  averaged, with the spacecraft attitude of the interval that contains each
  end time.
- Spectral weighting uses the trapezoid rule on the log-spaced energy grid
  (`enumbins` points). With few points and a steep spectrum, the weights are
  coarse.
- `Exposure::value(t, E)` (not used by the tool) throws `std::out_of_range`
  for `E` at or above the last grid energy or below the first, and for `t`
  outside the bin edges.
- The default `irfs` argument of the `Exposure` constructor, `"DC2"`, no
  longer exists.

**Exit status**
- Normal errors (missing file, bad XML, unknown `target`, missing `RATE`
  table) are thrown as exceptions and give exit status 1 through `st_app`.
  The segfault cases above give 139, which `sane`'s `GtApp.run()` in its
  default mode does **not** detect (see `src/sane/doc/architecture/`).

**Branches**
- `origin/addition_creator_fits` (unmerged) raises the default `chatter`
  from 2 to 3 so that the tool prints its version. `origin/Issue17` builds
  the library optionally static.

## Extension points

- **New spectral model for weighting:** include its header from
  `Likelihood` and add a `m_funcFactory->addFunc("Name", new …, false)` line
  to `GtExposure::prepareFunctionFactory`. Models that need data beyond
  `<parameter>` elements (like `FileFunction`) also need code in
  `prepareModel` to read it.
- **Other light-curve formats:** `getLcTimes` and `writeExposure` hard-code
  the `RATE` extension and the `TIME`/`TIMEDEL` columns.
- **Python access:** reviving `src/pyExposure.i` would need a
  `swig_add_library` target like `pyLikelihood`'s, an install rule, and the
  cast-helper fix noted above. Nothing in the tree depends on it today.

## Further reading

- `doc/release.notes`: history to 2015, including the 2009 removal of the
  SWIG build.
- `fermitools-fhelp/fhelp_files/gtexposure.txt`: user documentation.
  Its `irfs` example (`P6_V3_DIFFUSE`) is out of date.
- `src/Likelihood/` sources for `ScData` and `LikeExposure`, and
  `src/irfs/doc/architecture/` (IRF loading, PSF integrals; `Likelihood`
  docs when available).
- `src/dataSubselector/doc/architecture/` for DSS keywords and
  `CALDB_implied_irfs`.
- `src/evtbin/doc/architecture/` for the light curve format `gtbin` writes.
