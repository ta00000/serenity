/*
 * Copyright (c) 2021, Sam Atkins <atkinssj@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/TypeCasts.h>
#include <LibWeb/CSS/CSSImportRule.h>
#include <LibWeb/CSS/CSSMediaRule.h>
#include <LibWeb/CSS/CSSRuleList.h>
#include <LibWeb/CSS/CSSSupportsRule.h>
#include <LibWeb/DOM/ExceptionOr.h>

namespace Web::CSS {

CSSRuleList::CSSRuleList(NonnullRefPtrVector<CSSRule>&& rules)
    : m_rules(move(rules))
{
}

CSSRuleList::~CSSRuleList()
{
}

bool CSSRuleList::is_supported_property_index(u32 index) const
{
    // The object’s supported property indices are the numbers in the range zero to one less than the number of CSSRule objects represented by the collection.
    // If there are no such CSSRule objects, then there are no supported property indices.
    return index < m_rules.size();
}

// https://drafts.csswg.org/cssom/#insert-a-css-rule
DOM::ExceptionOr<unsigned> CSSRuleList::insert_a_css_rule(NonnullRefPtr<CSSRule> rule, u32 index)
{
    // 1. Set length to the number of items in list.
    auto length = m_rules.size();

    // 2. If index is greater than length, then throw an IndexSizeError exception.
    if (index > length)
        return DOM::IndexSizeError::create("CSS rule index out of bounds.");

    // NOTE: These steps don't apply since we're receiving a parsed rule.
    // 3. Set new rule to the results of performing parse a CSS rule on argument rule.
    // 4. If new rule is a syntax error, throw a SyntaxError exception.

    // FIXME: 5. If new rule cannot be inserted into list at the zero-index position index due to constraints specified by CSS, then throw a HierarchyRequestError exception. [CSS21]

    // FIXME: 6. If new rule is an @namespace at-rule, and list contains anything other than @import at-rules, and @namespace at-rules, throw an InvalidStateError exception.

    // 7. Insert new rule into list at the zero-indexed position index.
    m_rules.insert(index, move(rule));

    // 8. Return index.
    return index;
}

// https://drafts.csswg.org/cssom/#remove-a-css-rule
DOM::ExceptionOr<void> CSSRuleList::remove_a_css_rule(u32 index)
{
    // 1. Set length to the number of items in list.
    auto length = m_rules.size();

    // 2. If index is greater than or equal to length, then throw an IndexSizeError exception.
    if (index >= length)
        return DOM::IndexSizeError::create("CSS rule index out of bounds.");

    // 3. Set old rule to the indexth item in list.
    auto& old_rule = m_rules[index];

    // FIXME: 4. If old rule is an @namespace at-rule, and list contains anything other than @import at-rules, and @namespace at-rules, throw an InvalidStateError exception.
    (void)old_rule;

    // 5. Remove rule old rule from list at the zero-indexed position index.
    m_rules.remove(index);

    // FIXME: 6. Set old rule’s parent CSS rule and parent CSS style sheet to null.

    return {};
}

void CSSRuleList::for_each_effective_style_rule(Function<void(CSSStyleRule const&)> const& callback) const
{
    for (auto& rule : m_rules) {
        switch (rule.type()) {
        case CSSRule::Type::Style:
            callback(verify_cast<CSSStyleRule>(rule));
            break;
        case CSSRule::Type::Import: {
            auto const& import_rule = verify_cast<CSSImportRule>(rule);
            if (import_rule.has_import_result())
                import_rule.loaded_style_sheet()->for_each_effective_style_rule(callback);
            break;
        }
        case CSSRule::Type::Media:
            verify_cast<CSSMediaRule>(rule).for_each_effective_style_rule(callback);
            break;
        case CSSRule::Type::Supports:
            verify_cast<CSSSupportsRule>(rule).for_each_effective_style_rule(callback);
            break;
        case CSSRule::Type::__Count:
            VERIFY_NOT_REACHED();
        }
    }
}

bool CSSRuleList::for_first_not_loaded_import_rule(Function<void(CSSImportRule&)> const& callback)
{
    for (auto& rule : m_rules) {
        if (rule.type() == CSSRule::Type::Import) {
            auto& import_rule = verify_cast<CSSImportRule>(rule);
            if (!import_rule.has_import_result()) {
                callback(import_rule);
                return true;
            }

            if (import_rule.loaded_style_sheet()->for_first_not_loaded_import_rule(callback)) {
                return true;
            }
        } else if (rule.type() == CSSRule::Type::Media) {
            return verify_cast<CSSMediaRule>(rule).for_first_not_loaded_import_rule(callback);
        } else if (rule.type() == CSSRule::Type::Supports) {
            return verify_cast<CSSSupportsRule>(rule).for_first_not_loaded_import_rule(callback);
        }
    }

    return false;
}

void CSSRuleList::evaluate_media_queries(DOM::Window const& window)
{
    for (auto& rule : m_rules) {
        switch (rule.type()) {
        case CSSRule::Type::Style:
            break;
        case CSSRule::Type::Import: {
            auto& import_rule = verify_cast<CSSImportRule>(rule);
            if (import_rule.has_import_result())
                import_rule.loaded_style_sheet()->evaluate_media_queries(window);
            break;
        }
        case CSSRule::Type::Media: {
            auto& media_rule = verify_cast<CSSMediaRule>(rule);
            if (media_rule.evaluate(window))
                media_rule.css_rules().evaluate_media_queries(window);
            break;
        }
        case CSSRule::Type::Supports: {
            auto& supports_rule = verify_cast<CSSSupportsRule>(rule);
            if (supports_rule.condition_matches())
                supports_rule.css_rules().evaluate_media_queries(window);
            break;
        }
        case CSSRule::Type::__Count:
            VERIFY_NOT_REACHED();
        }
    }
}

}
