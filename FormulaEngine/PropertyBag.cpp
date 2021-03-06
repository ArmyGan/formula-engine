#include "Pch.h"

#include "Interfaces.h"
#include "Formula.h"
#include "Actions.h"
#include "TokenPool.h"
#include "PropertyBag.h"
#include "EventHandler.h"
#include "Scriptable.h"
#include "ScriptWorld.h"
#include "Functions.h"


void SimplePropertyBag::MaintainSorted() {
	std::sort(std::begin(m_bag), std::end(m_bag), [](const BagPair & l, const BagPair & r) {
		return l.first < r.first;
	});
}

void SimplePropertyBag::Set(unsigned token, const Result & value) {
	auto iter = std::remove_if(std::begin(m_bag), std::end(m_bag), [token](const std::pair<unsigned, ValueT> & item) {
		return item.first == token;
	});
	m_bag.erase(iter, m_bag.end());

	m_bag.emplace_back(token, value.payload.num.value);
	MaintainSorted();
}

Result SimplePropertyBag::ResolveNumber(const IFormulaContext & context, unsigned scope, unsigned token) const {
	Result ret;
	ret.code = RESULT_CODE_MISSING_DEFINITION;
	ret.payload.num.value = 0.0;

	if(scope != 0) {
		if(&context != this)
			return context.ResolveNumber(context, scope, token);

		return ret;
	}

	auto iter = std::lower_bound(std::begin(m_bag), std::end(m_bag), token, [](const BagPair & l, unsigned token) {
		return l.first < token;
	});
	if(iter != m_bag.end() && iter->first == token) {
		ret.code = RESULT_CODE_OK;
		ret.payload.num.value = iter->second;
	}

	return ret;
}

ListResult SimplePropertyBag::ResolveList(const IFormulaContext & context, unsigned scope, unsigned token) const {
	ref(context);
	ref(scope);
	ref(token);

	ListResult ret;
	ret.code = RESULT_CODE_MISSING_DEFINITION;
	return ret;
}



void FormulaPropertyBag::Clear() {
	m_bag.clear();
}

void FormulaPropertyBag::Flatten (SimplePropertyBag * bag, const ScopedPropertyBag * scopes) const {
	for (auto & pair : m_bag) {
		Result result = pair.second.Evaluate(scopes);
		if (result.code != RESULT_CODE_OK)
			assert(false);

		bag->Set(pair.first, result);
	}
}

void FormulaPropertyBag::MaintainSorted () {
	std::sort(std::begin(m_bag), std::end(m_bag), [](const BagPair & l, const BagPair & r) {
		return l.first < r.first;
	});
}

void FormulaPropertyBag::Set (unsigned token, Formula && formula) {
	for (auto & pair : m_bag) {
		if (pair.first == token) {
			pair.second = std::move(formula);
			return;
		}
	}

	m_bag.emplace_back(token, formula);
	MaintainSorted();
}

void FormulaPropertyBag::Set (unsigned token, const Formula & formula) {
	for (auto & pair : m_bag) {
		if (pair.first == token) {
			pair.second = formula;
			return;
		}
	}

	m_bag.emplace_back(token, formula);
	MaintainSorted();
}

Result FormulaPropertyBag::ResolveNumber (const IFormulaContext & context, unsigned scope, unsigned token) const {
	if (scope == 0) {
		for (const auto & pair : m_bag) {
			if (pair.first == token) {
				return pair.second.Evaluate(&context);
			}
		}
	}

	Result ret;
	ret.code = RESULT_CODE_MISSING_DEFINITION;
	ret.payload.num.value = 0.0;
	return ret;
}

ListResult FormulaPropertyBag::ResolveList (const IFormulaContext & context, unsigned scope, unsigned token) const {
	ref(context);
	ref(scope);
	ref(token);

	ListResult ret;
	ret.code = RESULT_CODE_MISSING_DEFINITION;
	return ret;
}


void FormulaPropertyBag::Set (unsigned token, const Result & value) {
	Formula f;
	if (value.type == RESULT_TYPE_SCALAR)
		f.Push(value.payload.num.value);
	else if (value.type == RESULT_TYPE_VECTOR2) {
		f.Push(value.payload.num.value);
		f.Push(value.payload.num.value2);
		f.Push(GetFunctionEvaluatorMakeVector());
	}

	Set(token, std::move(f));
}



void ScopeResolver::AddScope (unsigned token, const IFormulaContext & context) {
	m_bag.emplace_back(token, &context);
}

const IFormulaContext * ScopeResolver::GetScope (unsigned token) const {
	for (const auto & pair : m_bag) {
		if (pair.first == token)
			return pair.second;
	}

	return nullptr;
}

void ScopeResolver::Clear () {
	m_bag.clear();
}



ScopedPropertyBag::ScopedPropertyBag (ScriptWorld * world) {
	m_thisBag = &m_builtInBag;
	m_bindingBag = nullptr;
	m_world = world;
}

ScopedPropertyBag::ScopedPropertyBag (ScopedPropertyBag && other) {
	m_bindingBag = nullptr;

	std::swap(m_lists, other.m_lists);
	std::swap(m_builtInBag, other.m_builtInBag);
	std::swap(m_resolver, other.m_resolver);
	std::swap(m_bindingBag, other.m_bindingBag);
	std::swap(m_world, other.m_world);
	
	m_thisBag = &m_builtInBag;
	if (other.m_thisBag != &other.m_builtInBag)
		m_thisBag = other.m_thisBag;
}

ScopedPropertyBag::~ScopedPropertyBag () {
	for (auto & pair : m_lists) {
		for (auto & member : pair.second) {
			member->OnListMembershipRemoved(pair.first, this);
		}
	}

	delete m_bindingBag;
}


void ScopedPropertyBag::Clear () {
	m_lists.clear();
	m_builtInBag.Clear();
	m_resolver.Clear();

	m_thisBag = &m_builtInBag;
	
	delete m_bindingBag;
	m_bindingBag = nullptr;
}

void ScopedPropertyBag::InstantiateFrom(const ScopedPropertyBag & other) {
	m_resolver = other.m_resolver;
	m_builtInBag = other.m_builtInBag;
	m_thisBag = (other.m_thisBag == &other.m_builtInBag ? &m_builtInBag : other.m_thisBag);
}


void ScopedPropertyBag::ListAddEntry(unsigned listToken, Scriptable * entry) {
	m_lists[listToken].push_back(entry);
	entry->OnListMembershipAdded(listToken, this);
}

void ScopedPropertyBag::ListRemoveEntry(unsigned listToken, const Scriptable & entry) {
	auto iter = m_lists.find(listToken);
	if (iter == m_lists.end())
		return;

	auto memberiter = std::find(std::begin(iter->second), std::end(iter->second), &entry);
	if (memberiter == std::end(iter->second))
		return;

	iter->second.erase(memberiter);
}

const IFormulaContext * ScopedPropertyBag::ResolveContext (unsigned scope) const {
	if (scope == 0)
		return this;

	if (m_world) {
		TextPropertyBag * magic = m_world->GetMagicBag(scope);
		if (magic)
			return magic;

		Scriptable * target = m_world->GetScriptable(scope);
		if (target)
			return &target->GetScopes();
	}

	return m_resolver.GetScope(scope);
}

Result ScopedPropertyBag::ResolveNumber (const IFormulaContext & originalContext, unsigned scope, unsigned token) const {
	if (m_world && scope) {
		TextPropertyBag  * magic = m_world->GetMagicBag(scope);
		if (magic)
			return magic->ResolveNumber(originalContext, scope, token);

		Scriptable * target = m_world->GetScriptable(scope);
		if (target)
			return target->GetScopes().ResolveNumber(originalContext, 0, token);
	}

	auto resolved = scope ? m_resolver.GetScope(scope) : m_thisBag;
	if (resolved == nullptr) {
		Result ret;
		ret.code = RESULT_CODE_MISSING_DEFINITION;
		ret.payload.num.value = 0.0;
		return ret;
	}

	Result ret = resolved->ResolveNumber(originalContext, 0, token);
	if (ret.code != RESULT_CODE_OK) {
		if (m_bindingBag)
			return m_bindingBag->ResolveNumber(originalContext, 0, token);
	}

	return ret;
}

ListResult ScopedPropertyBag::ResolveList (const IFormulaContext & context, unsigned scope, unsigned token) const {
	ListResult ret;

	auto iter = m_lists.find(scope);
	if (iter != m_lists.end()) {
		ret.code = RESULT_CODE_OK;

		if (!iter->second.empty()) {
			ret.values.reserve(iter->second.size());

			for (const auto & scriptable : iter->second) {
				Result result = scriptable->GetScopes().ResolveNumber(context, 0, token);
				if (result.code != RESULT_CODE_OK) {
					ret.code = result.code;
					ret.values.clear();
					break;
				}

				ret.values.push_back(result.payload.num.value);
			}
		}

		return ret;
	}

	ret.code = RESULT_CODE_MISSING_DEFINITION;
	return ret;
}

void ScopedPropertyBag::SetFormula (unsigned token, const Formula & formula) {
	m_thisBag->Set(token, formula);
}

void ScopedPropertyBag::SetProperties (FormulaPropertyBag * refbag) {
	m_thisBag = refbag;
}

void ScopedPropertyBag::SetBindings (const BindingPropertyBag & refBag) {
	if (m_bindingBag)
		*m_bindingBag = refBag;
	else
		m_bindingBag = new BindingPropertyBag(refBag);
}


void ScopedPropertyBag::SetNamedBinding (unsigned token, Scriptable * binding) {
	m_namedBindings.emplace_back(token, binding);
}

Scriptable * ScopedPropertyBag::GetNamedBinding (unsigned token) const {
	for (const auto & pair : m_namedBindings) {
		if (pair.first == token)
			return pair.second;
	}

	return nullptr;
}

void ScopedPropertyBag::PopulateNamedBindings (ScopedPropertyBag * other) const {
	other->m_namedBindings = m_namedBindings;
}

void ScopedPropertyBag::Set (unsigned token, const Result & value) {
	m_thisBag->Set(token, value);
}

bool ScopedPropertyBag::ResolveToken (unsigned scope, unsigned token, std::string * out) const {
	if (m_world && scope) {
		TextPropertyBag  * magic = m_world->GetMagicBag(scope);
		if (magic) {
			*out = magic->GetLine(token);
			return true;
		}
	}

	if (!m_resolver.GetScope(scope))
		return false;

	return m_resolver.GetScope(scope)->ResolveToken(0, token, out);
}


BindingPropertyBag::BindingPropertyBag(const Scriptable * scriptable)
	: m_scriptable(scriptable)
{
}

Result BindingPropertyBag::ResolveNumber(const IFormulaContext & context, unsigned scope, unsigned token) const {
	return m_scriptable->ResolveBinding(context, scope, token);
}

ListResult BindingPropertyBag::ResolveList(const IFormulaContext & context, unsigned scope, unsigned token) const {
	ref(context);
	ref(scope);
	ref(token);

	ListResult ret;
	ret.code = RESULT_CODE_MISSING_DEFINITION;
	return ret;
}



TextPropertyBag::TextPropertyBag (unsigned scope) :
	m_scope(scope)
{
}


void TextPropertyBag::AddLine(unsigned token, const char * str) {
	m_bag[token] = str;
}


const char * TextPropertyBag::GetLine(unsigned token) const {
	auto iter = m_bag.find(token);
	if (iter == m_bag.end())
		return nullptr;

	return iter->second.c_str();
}


Result TextPropertyBag::ResolveNumber(const IFormulaContext & context, unsigned scope, unsigned token) const {
	ref(context);
	ref(scope);

	Result ret;
	ret.type = RESULT_TYPE_TOKEN;
	ret.code = RESULT_CODE_OK;
	ret.payload.txt.token = token;
	ret.payload.txt.scope = m_scope;

	return ret;
}

ListResult TextPropertyBag::ResolveList(const IFormulaContext & context, unsigned scope, unsigned token) const {
	ref(context);
	ref(scope);
	ref(token);

	ListResult ret;
	ret.code = RESULT_CODE_MISSING_DEFINITION;
	return ret;
}



void TokenPropertyBag::Set (unsigned token, const Result & value) {
	m_bag[token] = value;
}


Result TokenPropertyBag::ResolveNumber (const IFormulaContext & context, unsigned scope, unsigned token) const {
	ref(context);
	ref(scope);

	Result ret;

	auto iter = m_bag.find(token);
	if (iter == m_bag.end()) {
		ret.code = RESULT_CODE_MISSING_DEFINITION;
		ret.payload.num.value = 0.0;
		return ret;
	}

	ret = iter->second;
	return ret;
}


ListResult TokenPropertyBag::ResolveList (const IFormulaContext & context, unsigned scope, unsigned token) const {
	ref(context);
	ref(scope);
	ref(token);

	ListResult ret;
	ret.code = RESULT_CODE_MISSING_DEFINITION;
	return ret;
}


unsigned TokenPropertyBag::AddToken (const std::string & token) {
	return m_pool.AddToken(token);
}


bool TokenPropertyBag::ResolveToken (unsigned scope, unsigned token, std::string * out) const {
	ref(scope);
	*out = m_pool.GetStringFromToken(token);
	return true;
}

