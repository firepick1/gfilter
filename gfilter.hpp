#ifndef GFILTER_HPP
#define GFILTER_HPP

#include <ostream>
#include "jansson.h"

using namespace std;

namespace gfilter {

typedef class IGFilter {
	protected: const char * _name;
	public: const char * name() { return _name; }
	public: IGFilter() : _name("IGFilter") {};
  public: virtual int writeln(const char *value) =0;
} IGFilter, *IGFilterPtr;

typedef class GFilterBase : public IGFilter {
  protected: class IGFilter &_next;
  protected: GFilterBase(IGFilter &next) : _next(next){ _name="GFilterBase"; };
  public: virtual int writeln(const char *value) { _next.writeln(value); return 0;}
} GFilterBase, *GFilterBasePtr;

typedef class OStreamFilter : public IGFilter {
	private: ostream *pos;
	public: OStreamFilter(ostream &os) { pos = &os; };
	public: ~OStreamFilter() { pos->flush(); }
	public: virtual int writeln(const char *value);
} OStreamFilter, *OStreamFilterPtr;

typedef class DeltaFilter : public GFilterBase {
  public: DeltaFilter(IGFilter &next);
  public: virtual int writeln(const char *value);
} DeltaFilter, *DeltaFilterPtr;

} // namespace gfilter

#endif
